use winit::{
    event::ElementState,
    keyboard::{Key, KeyLocation, PhysicalKey},
};

/// Mirror of `winit::event::KeyEvent`, which cannot be constructed outside `winit`
/// (its `platform_specific` field is `pub(crate)`). Field names and types match winit's
/// six public fields, except `text`, which is a `String` instead of a `SmolStr`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct KeyEvent {
    pub physical_key: PhysicalKey,
    pub logical_key: Key,
    pub text: Option<String>,
    pub location: KeyLocation,
    pub state: ElementState,
    pub repeat: bool,
}
