use log;
use surge_sdk::{
    InstanceInfo, KeyEvent, SpriteCanvas, SubTextureInfo, SurgeApp,
    nalgebra::{Vector2, Vector4},
    winit::event::{DeviceId, ElementState, MouseButton, MouseScrollDelta, TouchPhase},
};

const NUM_FRAMES: u32 = 25;
const SECONDS_PER_FRAME: f32 = 1.0 / 20.0;

const MODULE_NAME: &str = "surge-mod-default";

/// Animation state lives here rather than in `thread_local!` statics, so a fresh instance (as
/// built on an F5 reload) starts from a clean slate.
pub struct AppDefault {
    /// X position of the moving sprite.
    position: f32,
    /// Current frame of the sprite-sheet animation.
    frame_counter: u32,
    /// Time accumulated towards the next animation frame.
    elapsed_time: f32,
}

#[unsafe(no_mangle)]
pub fn surge_register_app() -> Box<dyn SurgeApp> {
    Box::new(AppDefault {
        position: 0.0,
        frame_counter: 0,
        elapsed_time: 0.0,
    })
}

fn upload_assets(canvas: &mut dyn SpriteCanvas) {
    for asset_name in ["awesomeface.png", "awesomeanim.png"] {
        let asset_path = surge_sdk::resolve_asset_path(MODULE_NAME, asset_name);
        canvas.upload_texture(asset_path.to_str().unwrap()).unwrap();
    }
}

impl SurgeApp for AppDefault {
    fn on_load(&mut self, canvas: &mut dyn SpriteCanvas) {
        log::info!("Default app startup");

        upload_assets(canvas);
    }

    fn on_swapchain_recreate(&mut self, canvas: &mut dyn SpriteCanvas) {
        upload_assets(canvas);
    }

    fn update(&mut self, dt: f32, spdb: &mut dyn SpriteCanvas) {
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
        aii.position = Vector2::new(self.position, 150.0);
        self.position += 2.5;
        if self.position > 800.0 {
            self.position = -100.0;
        }

        aii.color_multiplier = Vector4::from_element(1.0);
        aii.z = 0.0;
        spdb.add_instance(&aii);

        // Test animation
        let frame_subtexture = SubTextureInfo::default()
            .origin(surge_sdk::nalgebra::Vector2::new(
                2 + self.frame_counter * (498 + 2),
                2,
            ))
            .extent(surge_sdk::nalgebra::Vector2::from_element(498));

        let anim_ii = InstanceInfo {
            position: Vector2::new(400.0, 0.0),
            scale: Vector2::from_element(100.0),
            z: 0.5,
            texture_id: 1,
            color_multiplier: Vector4::from_element(1.0),
            subtexture_info: Some(frame_subtexture),
        };
        spdb.add_instance(&anim_ii);

        // Carry the leftover time over instead of discarding it, so the animation does not
        // drift slow; a long frame may advance more than one animation frame.
        self.elapsed_time += dt;
        while self.elapsed_time >= SECONDS_PER_FRAME {
            self.elapsed_time -= SECONDS_PER_FRAME;
            self.frame_counter = (self.frame_counter + 1) % NUM_FRAMES;
        }
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

    fn mouse_wheel_event(
        &mut self,
        device_id: DeviceId,
        delta: MouseScrollDelta,
        phase: TouchPhase,
    ) {
        log::info!(
            "Mouse scroll event: ID = {:?} delta = {:?} phase = {:?}",
            device_id,
            delta,
            phase
        );
    }

    fn on_unload(&mut self) {
        log::info!("Default app shutdown");
    }
}
