use log;
use surge_core::module::SurgeModule;

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

    /// Update module state
    fn update(&self) {
        // Do nothing
    }

    /// Records module rendering commands
    fn draw(&self) {
        // Do nothing
    }

    /// Called when the module recieves a keyboard event
    fn keyboard_event(&self) {
        log::info!("Keyboard");
    }

    /// Called when the module recieves a mouse event
    fn mouse_event(&self) {
        log::info!("Mouse");
    }

    /// Called when engin is about to exit, and the module is about to quit
    fn on_unload(&self) {
        log::info!("Default module shutdown");
    }
}
