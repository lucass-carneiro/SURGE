use std::io::{self, Read, Write};

use serde::{Deserialize, Serialize, de::DeserializeOwned};
use winit::event::{ElementState, MouseButton, MouseScrollDelta, TouchPhase};

use crate::{
    input::KeyEvent,
    sprite::{CreateInfo, InstanceInfo},
};

/// Version of the player <-> module wire protocol. The module sends it in `Response::Handshake`
/// and the player compares against this same constant, so bumping it in one place is enough.
pub const WIRE_VERSION: u16 = 1;

/// Largest frame `read_message` will accept. A corrupt or hostile length prefix must not turn
/// into a multi-gigabyte allocation.
const MAX_FRAME_LEN: u32 = 64 * 1024 * 1024;

/// Player -> module.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Request {
    // `create_info` is pushed rather than pulled: the module has no way to ask the player anything
    // mid-turn.
    OnLoad { create_info: CreateInfo },
    OnSwapchainRecreate { create_info: CreateInfo },
    Frame { dt: f32, input_events: Vec<InputEvent> },
    Shutdown,
}

/// Module -> player.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Response {
    /// Sent unprompted by the module right after connecting.
    Handshake { wire_version: u16 },
    /// Reply to `OnLoad`, `OnSwapchainRecreate` and `Frame` alike.
    CanvasOps(CanvasOps),
    ShutdownAck,
}

/// Everything a module did to its `SpriteCanvas` during one turn.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct CanvasOps {
    /// Asset paths, in call order. `texture_id` is the proxy's *cumulative* upload counter, not
    /// this vec's index; the two coincide only when the canvas was freshly built at the start of
    /// the turn.
    pub texture_uploads: Vec<String>,
    /// Empty in practice for `OnLoad`/`OnSwapchainRecreate`, populated for `Frame`.
    pub instances: Vec<InstanceInfo>,
}

/// An input event, replayed into the module in wire order. `DeviceId` is deliberately absent:
/// the module side substitutes `DeviceId::dummy()`.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum InputEvent {
    Keyboard {
        event: KeyEvent,
        is_synthetic: bool,
    },
    MouseButton {
        state: ElementState,
        button: MouseButton,
    },
    MouseWheel {
        delta: MouseScrollDelta,
        phase: TouchPhase,
    },
}

/// Write `value` as one frame: a little-endian `u32` length, then the `postcard` encoding.
pub fn write_message<T: Serialize>(mut w: impl Write, value: &T) -> io::Result<()> {
    let bytes = postcard::to_stdvec(value).map_err(io::Error::other)?;
    let len = u32::try_from(bytes.len())
        .ok()
        .filter(|&len| len <= MAX_FRAME_LEN)
        .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidInput, "message too large"))?;
    w.write_all(&len.to_le_bytes())?;
    w.write_all(&bytes)?;
    w.flush()
}

/// Read exactly one frame written by [`write_message`]. A peer that closed the stream between
/// frames yields `ErrorKind::UnexpectedEof`, the same as one that closed it mid-frame.
pub fn read_message<T: DeserializeOwned>(mut r: impl Read) -> io::Result<T> {
    let mut len = [0u8; 4];
    r.read_exact(&mut len)?;
    let len = u32::from_le_bytes(len);
    if len > MAX_FRAME_LEN {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("frame length {len} exceeds limit"),
        ));
    }
    let mut buf = vec![0u8; len as usize];
    r.read_exact(&mut buf)?;
    postcard::from_bytes(&buf).map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))
}

#[cfg(test)]
mod tests {
    use std::os::unix::net::UnixStream;

    use winit::keyboard::{Key, KeyCode, KeyLocation, NamedKey, PhysicalKey};

    use super::*;
    use crate::sprite::{BlendingMode, SubTextureInfo, TextureFilteringLevel, TextureFilteringMode};

    fn create_info() -> CreateInfo {
        CreateInfo {
            blending_mode: BlendingMode::Alpha,
            texture_filtering_mode: TextureFilteringMode::Nearest,
            texture_filtering_level: TextureFilteringLevel::X16,
            max_sprites: 32,
            max_textures: 32,
            window_width: 500.0,
            window_height: 800.0,
        }
    }

    fn instance(texture_id: usize) -> InstanceInfo {
        InstanceInfo {
            position: nalgebra::Vector2::new(12.5, -3.0),
            scale: nalgebra::Vector2::new(64.0, 32.0),
            z: 0.25,
            texture_id,
            color_multiplier: nalgebra::Vector4::new(1.0, 0.5, 0.25, 1.0),
            subtexture_info: Some(
                SubTextureInfo::default()
                    .origin(nalgebra::Vector2::new(16, 32))
                    .extent(nalgebra::Vector2::new(8, 8)),
            ),
        }
    }

    fn key_event() -> KeyEvent {
        KeyEvent {
            physical_key: PhysicalKey::Code(KeyCode::ArrowRight),
            logical_key: Key::Named(NamedKey::ArrowRight),
            text: Some("→".to_string()),
            location: KeyLocation::Standard,
            state: ElementState::Pressed,
            repeat: false,
        }
    }

    fn all_input_events() -> Vec<InputEvent> {
        vec![
            InputEvent::Keyboard {
                event: key_event(),
                is_synthetic: false,
            },
            InputEvent::MouseButton {
                state: ElementState::Released,
                button: MouseButton::Other(9),
            },
            InputEvent::MouseWheel {
                delta: MouseScrollDelta::LineDelta(0.0, -1.0),
                phase: TouchPhase::Moved,
            },
            InputEvent::MouseWheel {
                delta: MouseScrollDelta::PixelDelta(winit::dpi::PhysicalPosition::new(1.5, 2.5)),
                phase: TouchPhase::Started,
            },
        ]
    }

    fn all_requests() -> Vec<Request> {
        vec![
            Request::OnLoad {
                create_info: create_info(),
            },
            Request::OnSwapchainRecreate {
                create_info: create_info(),
            },
            Request::Frame {
                dt: 0.016,
                input_events: all_input_events(),
            },
            Request::Frame {
                dt: 0.0,
                input_events: vec![],
            },
            Request::Shutdown,
        ]
    }

    fn all_responses() -> Vec<Response> {
        vec![
            Response::Handshake {
                wire_version: WIRE_VERSION,
            },
            Response::CanvasOps(CanvasOps {
                texture_uploads: vec!["a/b.png".to_string(), "c.png".to_string()],
                instances: vec![instance(0), instance(1)],
            }),
            Response::CanvasOps(CanvasOps::default()),
            Response::ShutdownAck,
        ]
    }

    /// Round-trips `value` through `postcard` and asserts the decoded value is identical. The wire
    /// types don't all implement `PartialEq` (`InstanceInfo` doesn't), so compare `Debug` output.
    fn assert_round_trips<T>(value: &T)
    where
        T: Serialize + DeserializeOwned + std::fmt::Debug,
    {
        let bytes = postcard::to_stdvec(value).unwrap();
        let decoded: T = postcard::from_bytes(&bytes).unwrap();
        assert_eq!(format!("{decoded:?}"), format!("{value:?}"));
    }

    #[test]
    fn every_request_and_response_variant_round_trips() {
        all_requests().iter().for_each(assert_round_trips);
        all_responses().iter().for_each(assert_round_trips);
    }

    #[test]
    fn framing_holds_message_boundaries_over_a_unix_stream() {
        let (mut tx, mut rx) = UnixStream::pair().unwrap();
        let requests = all_requests();

        // Written back to back with no reads in between: only the length prefix can keep the
        // frames apart on the receiving side.
        for request in &requests {
            write_message(&mut tx, request).unwrap();
        }
        for request in &requests {
            let got: Request = read_message(&mut rx).unwrap();
            assert_eq!(format!("{got:?}"), format!("{request:?}"));
        }

        // A peer that hangs up between frames reads as a clean EOF.
        drop(tx);
        let err = read_message::<Request>(&mut rx).unwrap_err();
        assert_eq!(err.kind(), io::ErrorKind::UnexpectedEof);
    }

    #[test]
    fn oversized_length_prefix_is_rejected_without_allocating() {
        let bytes = (MAX_FRAME_LEN + 1).to_le_bytes();
        let err = read_message::<Request>(&bytes[..]).unwrap_err();
        assert_eq!(err.kind(), io::ErrorKind::InvalidData);
    }
}
