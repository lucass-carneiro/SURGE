mod app;
mod canvas;
mod sprite;

// Re-exported so modules take exactly the versions the `SurgeApp` trait is written against.
pub use nalgebra;
pub use winit;

pub use app::SurgeApp;
pub use canvas::{CanvasError, SpriteCanvas};
pub use sprite::{
    BlendingMode, CreateInfo, InstanceInfo, SubTextureInfo, TextureFilteringLevel,
    TextureFilteringMode,
};
