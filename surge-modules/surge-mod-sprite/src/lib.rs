use log;
use surge_core::module::SurgeModule;
use winit::event::{DeviceId, ElementState, KeyEvent, MouseButton, MouseScrollDelta, TouchPhase};

pub struct ModuleSprite {}

impl ModuleSprite {
    pub fn new() -> Self {
        ModuleSprite {}
    }
}

impl SurgeModule for ModuleSprite {
    fn on_load(&self) {
        log::info!("Sprite demo module startup");
    }

    fn update(&self) {
        // Do nothing
    }

    fn draw(&self) {
        // Do nothing
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
        log::info!("Sprite demo module shutdown");
    }
}
