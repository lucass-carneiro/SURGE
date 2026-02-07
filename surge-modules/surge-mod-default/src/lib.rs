use log;
use nalgebra::{Vector2, Vector4};
use std::cell::RefCell;
use surge_core::{
    app::SurgeApp,
    vulkan::sprite_database::{InstanceInfo, SpriteDatabase, SubTextureInfo},
};
use winit::event::{DeviceId, ElementState, KeyEvent, MouseButton, MouseScrollDelta, TouchPhase};

const NUM_FRAMES: u32 = 25;
const SECONDS_PER_FRAME: f32 = 1.0 / 20.0;

pub struct AppDefault {}

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(AppDefault {})
}

impl SurgeApp for AppDefault {
    fn on_load(&mut self, spd: &mut SpriteDatabase) {
        surge_core::cli::init_env_logger();
        log::info!("Default app startup");

        spd.upload_texture("assets/awesomeface.png").unwrap();
        spd.upload_texture("assets/awesomeanim.png").unwrap();
    }

    fn on_swapchain_recreate(&self, spd: &mut SpriteDatabase) {
        spd.upload_texture("assets/awesomeface.png").unwrap();
        spd.upload_texture("assets/awesomeanim.png").unwrap();
    }

    fn update(&mut self, dt: f32, spdb: &mut SpriteDatabase) {
        // Test depth buffer
        let mut aii = InstanceInfo {
            position: Vector2::from_element(0.0),
            scale: Vector2::from_element(100.0),
            z: 0.5,
            texture_id: 0,
            color_multiplier: Vector4::from_element(1.0),
            subtexture_info: None,
        };
        spdb.add_instance(&aii);

        aii.position = Vector2::new(50.0, 0.0);
        aii.color_multiplier = Vector4::new(1.0, 0.0, 0.0, 1.0);
        aii.z = 0.0;
        spdb.add_instance(&aii);

        // Test alpha blending
        aii.position = Vector2::new(200.0, 0.0);
        aii.color_multiplier = Vector4::from_element(1.0);
        aii.z = 0.5;
        spdb.add_instance(&aii);

        aii.position = Vector2::new(250.0, 0.0);
        aii.color_multiplier = Vector4::new(0.0, 0.0, 1.0, 0.5);
        aii.z = 0.0;
        spdb.add_instance(&aii);

        // Test motion
        thread_local! {
            static POSITION: RefCell<f32> = const{ RefCell::new(0.0) };
        }

        POSITION.with_borrow_mut(|pos| {
            aii.position = Vector2::new(*pos, 150.0);
            *pos += 2.5;
            if *pos > 800.0 {
                *pos = -100.0;
            }
        });

        aii.color_multiplier = Vector4::from_element(1.0);
        aii.z = 0.0;
        spdb.add_instance(&aii);

        // Test animation
        thread_local! {
            static FRAME_COUNTER: RefCell<u32> = const{ RefCell::new(0) };
            static ELAPSED_TIME: RefCell<f32> = const{ RefCell::new(0.0f32) };
        }

        let frame_subtexture = FRAME_COUNTER.with_borrow(|fc| {
            SubTextureInfo::default()
                .origin(nalgebra::Vector2::new(2 + fc * (498 + 2), 2))
                .extent(nalgebra::Vector2::from_element(498))
        });

        let anim_ii = InstanceInfo {
            position: Vector2::new(400.0, 0.0),
            scale: Vector2::from_element(100.0),
            z: 0.5,
            texture_id: 1,
            color_multiplier: Vector4::from_element(1.0),
            subtexture_info: Some(frame_subtexture),
        };
        spdb.add_instance(&anim_ii);

        ELAPSED_TIME.with_borrow_mut(|et| {
            if *et >= SECONDS_PER_FRAME {
                *et = 0.0f32;
                FRAME_COUNTER.with_borrow_mut(|fc| {
                    *fc += 1;
                    *fc %= NUM_FRAMES
                });
            } else {
                *et += dt;
            }
        });
    }

    fn keyboard_event(&mut self, device_id: DeviceId, event: KeyEvent, is_synthetic: bool) {
        log::info!(
            "Keyboard event: ID = {:?} event = {:?} synthetic = {}",
            device_id,
            event,
            is_synthetic
        );
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

    fn mouse_wheel_event(&self, device_id: DeviceId, delta: MouseScrollDelta, phase: TouchPhase) {
        log::info!(
            "Mouse scroll event: ID = {:?} delta = {:?} phaase = {:?}",
            device_id,
            delta,
            phase
        );
    }

    fn on_unload(&self) {
        log::info!("Default app shutdown");
    }
}
