use std::io::{self, Read, Write};

use winit::event::DeviceId;

use crate::{
    app::SurgeApp,
    proxy::CanvasProxy,
    wire::{read_message, write_message, InputEvent, Request, Response, WIRE_VERSION},
};

/// The module-side entry point in dev mode: connect to the player's TCP loopback socket (port
/// given as `argv[1]`), then serve its requests until `Shutdown`. Every module's dev `main()` is a
/// call to this.
///
/// Exits the process instead of returning, so the module's own teardown never races the player's
/// reap: `on_unload` has already run by the time `drive` returns `Ok`.
pub fn run_dev_module(mut app: Box<dyn SurgeApp>) -> ! {
    let code = match connect().and_then(|socket| drive(&mut *app, socket)) {
        Ok(()) => 0,
        Err(e) => {
            log::error!("dev module loop ended abnormally: {e}");
            1
        }
    };
    std::process::exit(code)
}

fn connect() -> io::Result<std::net::TcpStream> {
    use std::net::TcpStream;

    let arg = std::env::args().nth(1).ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            "missing port (argv[1]); dev modules are spawned by surge-player",
        )
    })?;
    let port: u16 = arg.parse().map_err(|e| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            format!("invalid port {arg:?}: {e}"),
        )
    })?;
    let stream = TcpStream::connect(("127.0.0.1", port))?;
    // Matches the player side: a per-frame blocking round trip cannot tolerate Nagle's algorithm
    // holding a small write back.
    stream.set_nodelay(true)?;
    Ok(stream)
}

/// Handshake, then answer one `Request` at a time until `Shutdown`.
///
/// Split from `run_dev_module` so the loop can be tested over an in-process socket pair.
fn drive(app: &mut dyn SurgeApp, mut socket: impl Read + Write) -> io::Result<()> {
    write_message(
        &mut socket,
        &Response::Handshake {
            wire_version: WIRE_VERSION,
        },
    )?;

    // Built from the first `CreateInfo` the player pushes; there is nothing to construct it from
    // before that.
    let mut proxy: Option<CanvasProxy> = None;

    loop {
        let request: Request = read_message(&mut socket)?;
        log::debug!("request: {}", variant_name(&request));

        let ops = match request {
            Request::OnLoad { create_info } => {
                let canvas = cache_create_info(&mut proxy, create_info);
                app.on_load(canvas);
                canvas.take_ops()
            }
            Request::OnSwapchainRecreate { create_info } => {
                let canvas = cache_create_info(&mut proxy, create_info);
                app.on_swapchain_recreate(canvas);
                canvas.take_ops()
            }
            Request::Frame { dt, input_events } => {
                let canvas = proxy.as_mut().ok_or_else(|| {
                    io::Error::new(io::ErrorKind::InvalidData, "Frame received before OnLoad")
                })?;
                // Wire order, and before `update`: an input that arrived this frame is visible to
                // this frame's update, exactly as in a single process.
                for event in input_events {
                    dispatch_input(app, event);
                }
                app.update(dt, canvas);
                canvas.take_ops()
            }
            Request::Shutdown => {
                app.on_unload();
                write_message(&mut socket, &Response::ShutdownAck)?;
                return Ok(());
            }
        };
        write_message(&mut socket, &Response::CanvasOps(ops))?;
    }
}

/// Cache `create_info` in the proxy, creating it on first use. A recache also restarts the
/// proxy's upload counter, matching the database the player rebuilt just before sending it.
fn cache_create_info(
    proxy: &mut Option<CanvasProxy>,
    create_info: crate::CreateInfo,
) -> &mut CanvasProxy {
    match proxy {
        Some(proxy) => {
            proxy.cache_create_info(create_info);
            proxy
        }
        None => proxy.insert(CanvasProxy::new(create_info)),
    }
}

fn dispatch_input(app: &mut dyn SurgeApp, event: InputEvent) {
    // `DeviceId` never crosses the wire; neither module tells devices apart.
    let device = DeviceId::dummy();
    match event {
        InputEvent::Keyboard {
            event,
            is_synthetic,
        } => app.keyboard_event(device, event, is_synthetic),
        InputEvent::MouseButton { state, button } => app.mouse_button_event(device, state, button),
        InputEvent::MouseWheel { delta, phase } => app.mouse_wheel_event(device, delta, phase),
    }
}

fn variant_name(request: &Request) -> &'static str {
    match request {
        Request::OnLoad { .. } => "OnLoad",
        Request::OnSwapchainRecreate { .. } => "OnSwapchainRecreate",
        Request::Frame { .. } => "Frame",
        Request::Shutdown => "Shutdown",
    }
}

#[cfg(test)]
mod tests {
    use std::{
        net::{TcpListener, TcpStream},
        thread,
    };

    use winit::{
        event::{ElementState, MouseButton, MouseScrollDelta, TouchPhase},
        keyboard::{Key, KeyCode, KeyLocation, NamedKey, PhysicalKey},
    };

    use super::*;
    use crate::{
        canvas::SpriteCanvas,
        input::KeyEvent,
        sprite::{
            BlendingMode, CreateInfo, InstanceInfo, TextureFilteringLevel, TextureFilteringMode,
        },
    };

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

    fn key_event() -> KeyEvent {
        KeyEvent {
            physical_key: PhysicalKey::Code(KeyCode::ArrowUp),
            logical_key: Key::Named(NamedKey::ArrowUp),
            text: None,
            location: KeyLocation::Standard,
            state: ElementState::Pressed,
            repeat: false,
        }
    }

    /// Logs every hook it is called through, in call order, so tests can assert on sequencing.
    #[derive(Default)]
    struct RecordingApp {
        log: Vec<String>,
    }

    impl SurgeApp for RecordingApp {
        fn on_load(&mut self, canvas: &mut dyn SpriteCanvas) {
            let id = canvas.upload_texture("load.png").unwrap();
            self.log.push(format!("on_load upload={id}"));
        }

        fn on_swapchain_recreate(&mut self, canvas: &mut dyn SpriteCanvas) {
            let id = canvas.upload_texture("recreate.png").unwrap();
            self.log.push(format!(
                "on_swapchain_recreate upload={id} max_sprites={}",
                canvas.get_create_info().max_sprites
            ));
        }

        fn update(&mut self, dt: f32, canvas: &mut dyn SpriteCanvas) {
            canvas.add_instance(&InstanceInfo {
                position: nalgebra::Vector2::new(1.0, 2.0),
                scale: nalgebra::Vector2::new(3.0, 4.0),
                z: 0.0,
                texture_id: 0,
                color_multiplier: nalgebra::Vector4::new(1.0, 1.0, 1.0, 1.0),
                subtexture_info: None,
            });
            self.log.push(format!("update dt={dt}"));
        }

        fn keyboard_event(&mut self, _: DeviceId, event: KeyEvent, is_synthetic: bool) {
            self.log.push(format!(
                "keyboard {:?} synthetic={is_synthetic}",
                event.state
            ));
        }

        fn mouse_button_event(&mut self, _: DeviceId, state: ElementState, button: MouseButton) {
            self.log.push(format!("mouse_button {state:?} {button:?}"));
        }

        fn mouse_wheel_event(&mut self, _: DeviceId, _: MouseScrollDelta, phase: TouchPhase) {
            self.log.push(format!("mouse_wheel {phase:?}"));
        }

        fn on_unload(&mut self) {
            self.log.push("on_unload".to_string());
        }
    }

    /// A connected pair of loopback `TcpStream`s, standing in for `UnixStream::pair()` (not
    /// available cross-platform) since `drive` only needs something `Read + Write`.
    fn tcp_pair() -> (TcpStream, TcpStream) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let addr = listener.local_addr().unwrap();
        let connecting = thread::spawn(move || TcpStream::connect(addr).unwrap());
        let (accepted, _) = listener.accept().unwrap();
        (accepted, connecting.join().unwrap())
    }

    /// Runs `script` on a second thread as the player, against `drive()` on this one. Returns
    /// whatever the script returns, plus `drive`'s own result.
    fn run_against<T: Send + 'static>(
        app: &mut dyn SurgeApp,
        script: impl FnOnce(TcpStream) -> T + Send + 'static,
    ) -> (io::Result<()>, T) {
        let (module_end, player_end) = tcp_pair();
        let player = thread::spawn(move || script(player_end));
        let result = drive(app, module_end);
        (result, player.join().unwrap())
    }

    fn round_trip(player: &mut TcpStream, request: &Request) -> Response {
        write_message(&mut *player, request).unwrap();
        read_message(&mut *player).unwrap()
    }

    fn expect_ops(response: Response) -> crate::CanvasOps {
        match response {
            Response::CanvasOps(ops) => ops,
            other => panic!("expected CanvasOps, got {other:?}"),
        }
    }

    #[test]
    fn scripted_session_covers_every_request() {
        let mut app = RecordingApp::default();

        let (result, (handshake, on_load, frame, recreate, shutdown)) =
            run_against(&mut app, |mut player| {
                let handshake: Response = read_message(&mut player).unwrap();
                let on_load = round_trip(
                    &mut player,
                    &Request::OnLoad {
                        create_info: create_info(),
                    },
                );
                let frame = round_trip(
                    &mut player,
                    &Request::Frame {
                        dt: 0.5,
                        input_events: vec![],
                    },
                );
                let recreate = round_trip(
                    &mut player,
                    &Request::OnSwapchainRecreate {
                        create_info: CreateInfo {
                            max_sprites: 64,
                            ..create_info()
                        },
                    },
                );
                let shutdown = round_trip(&mut player, &Request::Shutdown);
                (handshake, on_load, frame, recreate, shutdown)
            });

        result.unwrap();

        assert!(matches!(
            handshake,
            Response::Handshake { wire_version } if wire_version == WIRE_VERSION
        ));

        let on_load = expect_ops(on_load);
        assert_eq!(on_load.texture_uploads, ["load.png"]);
        assert!(on_load.instances.is_empty());

        let frame = expect_ops(frame);
        assert!(frame.texture_uploads.is_empty());
        assert_eq!(frame.instances.len(), 1);

        // The recreate hook sees the newly pushed `CreateInfo`, and its upload index restarts at
        // zero rather than continuing from `on_load`'s.
        let recreate = expect_ops(recreate);
        assert_eq!(recreate.texture_uploads, ["recreate.png"]);
        assert!(recreate.instances.is_empty());

        assert!(matches!(shutdown, Response::ShutdownAck));

        assert_eq!(
            app.log,
            [
                "on_load upload=0",
                "update dt=0.5",
                "on_swapchain_recreate upload=0 max_sprites=64",
                "on_unload",
            ]
        );
    }

    #[test]
    fn frame_input_is_dispatched_in_wire_order_before_update() {
        let mut app = RecordingApp::default();

        let (result, ()) = run_against(&mut app, |mut player| {
            let _handshake: Response = read_message(&mut player).unwrap();
            round_trip(
                &mut player,
                &Request::OnLoad {
                    create_info: create_info(),
                },
            );
            expect_ops(round_trip(
                &mut player,
                &Request::Frame {
                    dt: 0.25,
                    input_events: vec![
                        InputEvent::MouseButton {
                            state: ElementState::Pressed,
                            button: MouseButton::Left,
                        },
                        InputEvent::Keyboard {
                            event: key_event(),
                            is_synthetic: true,
                        },
                        InputEvent::MouseWheel {
                            delta: MouseScrollDelta::LineDelta(0.0, 1.0),
                            phase: TouchPhase::Moved,
                        },
                    ],
                },
            ));
            round_trip(&mut player, &Request::Shutdown);
        });

        result.unwrap();
        assert_eq!(
            app.log,
            [
                "on_load upload=0",
                "mouse_button Pressed Left",
                "keyboard Pressed synthetic=true",
                "mouse_wheel Moved",
                "update dt=0.25",
                "on_unload",
            ]
        );
    }

    #[test]
    fn upload_index_continues_across_frames_without_a_new_create_info() {
        struct UploadsEveryFrame(Vec<usize>);
        impl SurgeApp for UploadsEveryFrame {
            fn update(&mut self, _: f32, canvas: &mut dyn SpriteCanvas) {
                self.0.push(canvas.upload_texture("late.png").unwrap());
            }
        }
        let mut app = UploadsEveryFrame(vec![]);

        let (result, ()) = run_against(&mut app, |mut player| {
            let _handshake: Response = read_message(&mut player).unwrap();
            round_trip(
                &mut player,
                &Request::OnLoad {
                    create_info: create_info(),
                },
            );
            for _ in 0..2 {
                round_trip(
                    &mut player,
                    &Request::Frame {
                        dt: 0.0,
                        input_events: vec![],
                    },
                );
            }
            round_trip(&mut player, &Request::Shutdown);
        });

        result.unwrap();
        assert_eq!(app.0, [0, 1]);
    }

    #[test]
    fn frame_before_on_load_is_a_protocol_error() {
        let mut app = RecordingApp::default();

        let (result, ()) = run_against(&mut app, |mut player| {
            let _handshake: Response = read_message(&mut player).unwrap();
            write_message(
                &mut player,
                &Request::Frame {
                    dt: 0.0,
                    input_events: vec![],
                },
            )
            .unwrap();
        });

        assert_eq!(result.unwrap_err().kind(), io::ErrorKind::InvalidData);
        assert!(app.log.is_empty());
    }

    #[test]
    fn player_hanging_up_without_shutdown_is_an_error() {
        let mut app = RecordingApp::default();

        let (result, ()) = run_against(&mut app, |mut player| {
            let _handshake: Response = read_message(&mut player).unwrap();
            // Dropping the stream on return is the hang-up.
        });

        assert_eq!(result.unwrap_err().kind(), io::ErrorKind::UnexpectedEof);
        assert!(!app.log.contains(&"on_unload".to_string()));
    }
}
