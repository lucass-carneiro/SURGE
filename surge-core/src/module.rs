use crate::vulkan::sprite_database::SpriteDatabase;
use winit::event::{DeviceId, ElementState, KeyEvent, MouseButton, MouseScrollDelta, TouchPhase};

/// Defines a SURGE application
pub trait SurgeModule {
    /// Called when the application starts
    fn on_load(&self, spdb: &mut SpriteDatabase);

    /// Update application state
    fn update(&self, dt: f32, spdb: &mut SpriteDatabase);

    /// Records application rendering commands
    fn draw(&self);

    /// Called when the application recieves a keyboard event
    fn keyboard_event(&self, device_id: DeviceId, event: KeyEvent, is_synthetic: bool);

    /// Called when the application recieves a mouse button event
    fn mouse_button_event(&self, device_id: DeviceId, state: ElementState, button: MouseButton);

    /// Called when the application recieves a mouse wheel event
    fn mouse_wheel_event(&self, device_id: DeviceId, delta: MouseScrollDelta, phase: TouchPhase);

    /// Called when engin is about to exit, and the application is about to quit
    fn on_unload(&self);
}
