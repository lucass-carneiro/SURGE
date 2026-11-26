mod app;
mod assets;
mod canvas;
pub mod config;
mod dev;
mod input;
mod logging;
mod proxy;
mod shaping;
mod sprite;
mod text;
mod wire;

// Re-exported so modules take exactly the versions the `SurgeApp` trait is written against.
pub use nalgebra;
pub use winit;

pub use app::SurgeApp;
pub use assets::resolve_asset_path;
pub use canvas::{CanvasError, SpriteCanvas};
pub use dev::run_dev_module;
pub use input::KeyEvent;
pub use logging::init_env_logger;
pub use proxy::CanvasProxy;
pub use shaping::{Font, LineMetrics, ShapedText, TextAnchor, TextCanvasExt};
#[doc(hidden)]
pub use shaping::{anchor_offset, px_per_unit, select_vertical_metrics};
pub use sprite::{
    BlendingMode, CreateInfo, InstanceInfo, SubTextureInfo, TextureFilteringLevel,
    TextureFilteringMode,
};
pub use text::{PositionedGlyph, TextRun};
pub use wire::{
    CanvasOps, InputEvent, Request, Response, WIRE_VERSION, read_message, write_message,
};
