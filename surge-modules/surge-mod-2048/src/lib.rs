use surge_sdk::{
    InstanceInfo, KeyEvent, SpriteCanvas, SurgeApp,
    nalgebra::{Vector2 as Vec2, Vector4 as Vec4},
    winit::{
        event::{DeviceId, ElementState, MouseButton},
        keyboard::{KeyCode, PhysicalKey},
    },
};

mod app_2048;
mod board_geometry;
mod move_ops;
mod new_game_button;
mod piece;

use app_2048::{App2048, MoveDirection};
use board_geometry::BoardGeometry;

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(App2048::new())
}

const MODULE_NAME: &str = "surge-mod-2048";

const IMAGE_ASSETS: [&str; 13] = [
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
    "button.png",
];

fn upload_board_assets(canvas: &mut dyn SpriteCanvas) {
    for asset_name in IMAGE_ASSETS {
        let asset_path = surge_sdk::resolve_asset_path(MODULE_NAME, asset_name);
        canvas.upload_texture(asset_path.to_str().unwrap()).unwrap();
    }
}

const GAME_OVER_TINT: f32 = 0.35;

fn add_board_sprite(spdb: &mut dyn SpriteCanvas, bg: &BoardGeometry, tint: Vec4<f32>) {
    let scale = Vec2::new(
        spdb.get_create_info().window_width,
        spdb.get_create_info().window_height,
    );

    let board_ii = InstanceInfo {
        position: Vec2::from_element(0.0),
        scale,
        z: bg.get_board_z(),
        texture_id: 0,
        color_multiplier: tint,
        subtexture_info: None,
    };

    spdb.add_instance(&board_ii);
}

impl SurgeApp for App2048 {
    fn on_load(&mut self, spdb: &mut dyn SpriteCanvas) {
        log::info!("2048 startup");
        upload_board_assets(spdb);
    }

    fn on_swapchain_recreate(&mut self, spdb: &mut dyn SpriteCanvas) {
        upload_board_assets(spdb);
    }

    fn update(&mut self, dt: f32, spdb: &mut dyn SpriteCanvas) {
        // Advance the game state first, so the sprites submitted below show this frame's
        // positions rather than the previous frame's.
        self.step(dt);

        // Add sprites, dimmed once no move is left.
        let tint = if self.is_game_over() {
            Vec4::new(GAME_OVER_TINT, GAME_OVER_TINT, GAME_OVER_TINT, 1.0)
        } else {
            Vec4::from_element(1.0)
        };

        add_board_sprite(spdb, self.get_board_geometry(), tint);

        for piece in self.get_live_pieces() {
            let mut instance = piece.get_sprite_instance_info(self.get_board_geometry());
            instance.color_multiplier = tint;
            spdb.add_instance(&instance);
        }

        // Never dimmed: after a game over it is the one thing left to click.
        spdb.add_instance(&self.get_new_game_button().get_sprite_instance_info());
    }

    fn keyboard_event(&mut self, _: DeviceId, event: KeyEvent, _: bool) {
        if event.state != ElementState::Pressed || event.repeat {
            return;
        }

        let dir = match event.physical_key {
            PhysicalKey::Code(KeyCode::ArrowUp) => MoveDirection::Up,
            PhysicalKey::Code(KeyCode::ArrowDown) => MoveDirection::Down,
            PhysicalKey::Code(KeyCode::ArrowLeft) => MoveDirection::Left,
            PhysicalKey::Code(KeyCode::ArrowRight) => MoveDirection::Right,
            _ => return,
        };

        self.start_move(dir);
    }

    fn cursor_moved_event(&mut self, _: DeviceId, position: Vec2<f32>) {
        self.get_new_game_button_mut().cursor_moved(position);
    }

    fn mouse_button_event(&mut self, _: DeviceId, state: ElementState, button: MouseButton) {
        if self.get_new_game_button_mut().mouse_button(state, button) {
            log::info!("2048: new game");
            self.restart();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn texture_ids_match_the_upload_order() {
        assert_eq!(IMAGE_ASSETS[0], "board.png");
        assert_eq!(IMAGE_ASSETS[piece::MAX_VALUE], "pieces_2048.png");
        assert_eq!(IMAGE_ASSETS[new_game_button::TEXTURE_ID], "button.png");
    }

    #[test]
    fn every_image_asset_exists() {
        // `cargo test` runs from the crate directory, where `resolve_asset_path`'s
        // repo-root-relative fallback does not apply, so look next to the manifest instead.
        let assets = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("assets");
        for name in IMAGE_ASSETS {
            let path = assets.join(name);
            assert!(path.is_file(), "missing {}", path.display());
        }
    }
}
