use log;
use nalgebra::{Vector2 as Vec2, Vector4 as Vec4};

use surge_core::{
    app::SurgeApp,
    vulkan::sprite_database::{DecodedTexture, InstanceInfo, SpriteDatabase},
};
use winit::{
    event::{DeviceId, ElementState, KeyEvent, MouseButton},
    keyboard::{KeyCode, PhysicalKey},
};

mod app_2048;
mod board_geometry;
mod piece;
mod right_ops;

use app_2048::{App2048, BoardState, MoveDirection};
use board_geometry::BoardGeometry;

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(App2048::new())
}

/// Decodes every board/piece PNG from disk once. The result is cached by `App2048` so that
/// `on_swapchain_recreate` (which rebuilds the `SpriteDatabase`, GPU textures included, from
/// scratch) can re-upload without re-reading and re-decoding the files every time.
const MODULE_NAME: &str = "surge-mod-2048";

pub(crate) fn decode_image_assets() -> Vec<DecodedTexture> {
    let image_asset_names: [&'static str; 12] = [
        "board.png",
        "pieces_2.png",
        "pieces_4.png",
        "pieces_8.png",
        "pieces_16.png",
        "pieces_32.png",
        "pieces_64.png",
        "pieces_128.png",
        "pieces_256.png",
        "pieces_512.png",
        "pieces_1024.png",
        "pieces_2048.png",
    ];

    image_asset_names
        .iter()
        .map(|asset_name| {
            let asset_path = surge_core::assets::resolve_asset_path(MODULE_NAME, asset_name);
            SpriteDatabase::decode_texture_file(asset_path.to_str().unwrap()).unwrap()
        })
        .collect()
}

fn upload_image_assets(spdb: &mut SpriteDatabase, textures: &[DecodedTexture]) {
    for texture in textures {
        spdb.upload_decoded_texture(texture).unwrap();
    }
}

fn add_board_sprite(spdb: &mut SpriteDatabase, bg: &BoardGeometry) {
    let scale = Vec2::new(
        spdb.get_create_info().window_width,
        spdb.get_create_info().window_height,
    );

    let board_ii = InstanceInfo {
        position: Vec2::from_element(0.0),
        scale,
        z: bg.get_board_z(),
        texture_id: 0,
        color_multiplier: Vec4::from_element(1.0),
        subtexture_info: None,
    };

    spdb.add_instance(&board_ii);
}

impl SurgeApp for App2048 {
    fn on_load(&mut self, spdb: &mut SpriteDatabase) {
        surge_core::cli::init_env_logger();
        log::info!("2048 startup");
        upload_image_assets(spdb, self.get_cached_textures());
    }

    fn on_swapchain_recreate(&mut self, spdb: &mut SpriteDatabase) {
        upload_image_assets(spdb, self.get_cached_textures());
    }

    fn update(&mut self, _: f32, spdb: &mut SpriteDatabase) {
        // Add sprites
        add_board_sprite(spdb, self.get_board_geometry());

        for piece in self.get_live_pieces() {
            spdb.add_instance(&piece.get_sprite_instance_info(self.get_board_geometry()));
        }

        // Process game state
        let current_state = match self.get_board_states().front() {
            Some(s) => s,
            None => {
                self.push_state(BoardState::Idle);
                return;
            }
        };

        match current_state {
            BoardState::Idle => return,
            BoardState::Move(merge_direction) => {
                log::info!("Board merge {:?}", merge_direction);

                // Move all pieces that need moving. Pop the state if no more pieces are moving
                if self.update_live_pieces_pos() {
                    self.pop_state();
                }
            }
            BoardState::Resolve => {
                log::info!("Board resolve");
                self.pop_state();
            }
        }
    }

    fn keyboard_event(&mut self, _: DeviceId, event: KeyEvent, _: bool) {
        match event {
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowUp),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    self.push_state(BoardState::Resolve);
                    self.push_state(BoardState::Move(MoveDirection::Up));
                    self.push_state(BoardState::Idle);
                    self.pop_state();
                }
            }
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowDown),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    self.push_state(BoardState::Move(MoveDirection::Down));
                    self.push_state(BoardState::Resolve);
                    self.push_state(BoardState::Idle);
                    self.pop_state();
                }
            }
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowLeft),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    self.push_state(BoardState::Move(MoveDirection::Left));
                    self.push_state(BoardState::Resolve);
                    self.push_state(BoardState::Idle);
                    self.pop_state();
                }
            }
            KeyEvent {
                physical_key: PhysicalKey::Code(KeyCode::ArrowRight),
                state: ElementState::Pressed,
                repeat: false,
                ..
            } => {
                if self.board_idle() {
                    // Update game state queue
                    self.push_state(BoardState::Move(MoveDirection::Right));
                    self.push_state(BoardState::Resolve);
                    self.push_state(BoardState::Idle);
                    self.pop_state();

                    // Update piece target positions
                    self.set_move_target_right();
                }
            }
            _ => return,
        }
    }

    fn mouse_button_event(
        &mut self,
        device_id: DeviceId,
        state: ElementState,
        button: MouseButton,
    ) {
        log::info!(
            "Mouse event: ID = {:?} event = {:?} button = {:?}",
            device_id,
            state,
            button
        );
    }
}
