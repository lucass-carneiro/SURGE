use log;
use nalgebra::{Vector2, Vector4};
use surge_core::{
    app::SurgeApp,
    vulkan::sprite_database::{InstanceInfo, SpriteDatabase},
};

pub struct App2048 {}

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(App2048 {})
}

const BOARD_Z: f32 = 0.0;

const IMAGE_ASSET_PATHS: [&'static str; 12] = [
    "surge-modules/surge-mod-2048/assets/board.png",
    "surge-modules/surge-mod-2048/assets/pieces_2.png",
    "surge-modules/surge-mod-2048/assets/pieces_4.png",
    "surge-modules/surge-mod-2048/assets/pieces_8.png",
    "surge-modules/surge-mod-2048/assets/pieces_16.png",
    "surge-modules/surge-mod-2048/assets/pieces_32.png",
    "surge-modules/surge-mod-2048/assets/pieces_64.png",
    "surge-modules/surge-mod-2048/assets/pieces_128.png",
    "surge-modules/surge-mod-2048/assets/pieces_256.png",
    "surge-modules/surge-mod-2048/assets/pieces_512.png",
    "surge-modules/surge-mod-2048/assets/pieces_1024.png",
    "surge-modules/surge-mod-2048/assets/pieces_2048.png",
];

fn load_image_assets(spdb: &mut SpriteDatabase) {
    for asset_path in IMAGE_ASSET_PATHS {
        spdb.upload_texture(asset_path).unwrap();
    }
}

fn update_board(spdb: &mut SpriteDatabase) {
    let scale = nalgebra::Vector2::new(
        spdb.get_create_info().window_width,
        spdb.get_create_info().window_height,
    );

    let board_ii = InstanceInfo {
        position: Vector2::from_element(0.0),
        scale,
        z: BOARD_Z,
        texture_id: 0,
        color_multiplier: Vector4::from_element(1.0),
        subtexture_info: None,
    };

    spdb.add_instance(&board_ii);
}

impl SurgeApp for App2048 {
    fn on_load(&self, spdb: &mut SpriteDatabase) {
        surge_core::cli::init_env_logger();
        log::info!("2048 startup");
        load_image_assets(spdb);
    }

    fn on_swapchain_recreate(&self, spdb: &mut SpriteDatabase) {
        load_image_assets(spdb);
    }

    fn update(&self, _: f32, spdb: &mut SpriteDatabase) {
        update_board(spdb);
    }

    fn on_unload(&self) {
        log::info!("2048 shutdown");
    }
}
