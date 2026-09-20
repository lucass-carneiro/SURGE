use winit::event::{DeviceId, ElementState, MouseButton, MouseScrollDelta, TouchPhase};

use crate::{canvas::SpriteCanvas, input::KeyEvent};

/// Defines a SURGE application
pub trait SurgeApp {
    /// Called when the application starts
    fn on_load(&mut self, _: &mut dyn SpriteCanvas) {}

    /// Called if the swapchain is recreated
    fn on_swapchain_recreate(&mut self, _: &mut dyn SpriteCanvas) {}

    /// Update application state
    fn update(&mut self, _: f32, _: &mut dyn SpriteCanvas) {}

    /// Called when the application recieves a keyboard event
    fn keyboard_event(&mut self, _: DeviceId, _: KeyEvent, _: bool) {}

    /// Called when the application recieves a mouse button event
    fn mouse_button_event(&mut self, _: DeviceId, _: ElementState, _: MouseButton) {}

    /// Called when the application recieves a mouse wheel event
    fn mouse_wheel_event(&mut self, _: DeviceId, _: MouseScrollDelta, _: TouchPhase) {}

    /// Called when engin is about to exit, and the application is about to quit
    fn on_unload(&mut self) {}
}
