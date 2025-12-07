use log;
use surge_core::module::SurgeModule;
use winit::event::{DeviceId, ElementState, KeyEvent, MouseButton, MouseScrollDelta, TouchPhase};

pub struct ModuleDefault {}

impl ModuleDefault {
    pub fn new() -> Self {
        ModuleDefault {}
    }
}

impl SurgeModule for ModuleDefault {
    fn on_load(&self) {
        log::info!("Default module startup");
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

    fn mouse_wheel_event(
        &self,
        device_id: DeviceId,
        delta: winit::event::MouseScrollDelta,
        phase: winit::event::TouchPhase,
    ) {
        log::info!(
            "Mouse scroll event: ID = {:?} delta = {:?} phaase = {:?}",
            device_id,
            delta,
            phase
        );
    }

    fn on_unload(&self) {
        log::info!("Default module shutdown");
    }
}
