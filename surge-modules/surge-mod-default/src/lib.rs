use log;
use nalgebra::{Vector2, Vector4};
use surge_core::{
    app::SurgeApp,
    vulkan::sprite_database::{InstanceInfo, SpriteDatabase},
};
use winit::event::{DeviceId, ElementState, KeyEvent, MouseButton, MouseScrollDelta, TouchPhase};

pub struct AppDefault {}

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(AppDefault {})
}

impl SurgeApp for AppDefault {
    fn on_load(&self, spd: &mut SpriteDatabase) {
        surge_core::cli::init_env_logger();
        log::info!("Default app startup");

        spd.upload_texture("awesomeface.png").unwrap();
    }

    fn update(&self, _: f32, spdb: &mut SpriteDatabase) {
        // Test depth buffer
        let mut aii = InstanceInfo {
            position: Vector2::from_element(0.0),
            scale: Vector2::from_element(100.0),
            z: 0.5,
            texture_id: 0,
            color_multiplier: Vector4::from_element(1.0),
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
        static mut POS: f32 = 0.0;

        unsafe {
            aii.position = Vector2::new(POS, 150.0);
            POS += 2.5;
            if POS > 800.0 {
                POS = -100.0;
            }
        }
        aii.color_multiplier = Vector4::from_element(1.0);
        aii.z = 0.0;
        spdb.add_instance(&aii);
    }

    fn keyboard_event(&self, device_id: DeviceId, event: KeyEvent, is_synthetic: bool) {
        log::info!(
            "Keyboard event: ID = {:?} event = {:?} synthetic = {}",
            device_id,
            event,
            is_synthetic
        );
    }

    fn mouse_button_event(&self, device_id: DeviceId, state: ElementState, button: MouseButton) {
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
