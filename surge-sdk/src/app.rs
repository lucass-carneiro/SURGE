use nalgebra::Vector2;
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

    /// Called when the application receives a keyboard event
    fn keyboard_event(&mut self, _: DeviceId, _: KeyEvent, _: bool) {}

    /// Called when the application receives a mouse button event
    fn mouse_button_event(&mut self, _: DeviceId, _: ElementState, _: MouseButton) {}

    /// Called when the cursor moves over the window. `position` is in design-resolution
    /// coordinates, the same space sprites are placed in (origin top-left, +y down), so a module
    /// can hit-test it against its sprites directly. Over the letterbox bars it lies outside
    /// `0..window_width` / `0..window_height`.
    fn cursor_moved_event(&mut self, _: DeviceId, _position: Vector2<f32>) {}

    /// Called when the application receives a mouse wheel event
    fn mouse_wheel_event(&mut self, _: DeviceId, _: MouseScrollDelta, _: TouchPhase) {}

    /// Called when the engine is about to exit, and the application is about to quit
    fn on_unload(&mut self) {}

    /// Replaces the running module with a fresh instance of itself. Returns `true` if the player
    /// must rebuild its canvas and swapchain afterwards, since the new module has uploaded
    /// nothing yet. Apps that cannot be reloaded keep the default.
    fn reload(&mut self) -> bool {
        false
    }

    /// Whether the app has stopped working and can only be recovered by `reload`. The player
    /// stops running frames while this is `true`. Apps that cannot fault keep the default.
    fn is_faulted(&self) -> bool {
        false
    }
}
