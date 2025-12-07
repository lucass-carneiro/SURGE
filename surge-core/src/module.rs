/// Defines a SURGE application
pub trait SurgeModule {
    /// Called when the application starts
    fn on_load(&self);

    /// Update application state
    fn update(&self);

    /// Records application rendering commands
    fn draw(&self);

    /// Called when the application recieves a keyboard event
    fn keyboard_event(&self);

    /// Called when the application recieves a mouse event
    fn mouse_event(&self);

    /// Called when engin is about to exit, and the application is about to quit
    fn on_unload(&self);
}
