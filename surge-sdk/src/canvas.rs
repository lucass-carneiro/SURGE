use std::fmt;

use crate::sprite::{CreateInfo, InstanceInfo};
use crate::text::TextRun;

/// Opaque, message-carrying error returned by [`SpriteCanvas`] operations.
///
/// Deliberately not a reuse of any Vulkan-side error type: `surge-sdk` stays Vulkan-free, so
/// backends convert their own errors into a message here.
#[derive(Debug, Clone)]
pub struct CanvasError {
    message: String,
}

impl CanvasError {
    pub fn new(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
        }
    }

    pub fn message(&self) -> &str {
        &self.message
    }
}

impl fmt::Display for CanvasError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.message)
    }
}

impl std::error::Error for CanvasError {}

/// The module-facing sprite API: everything a module may call on the renderer.
pub trait SpriteCanvas {
    /// Queue one sprite for this frame. Occupancy resets after every draw, exactly as
    /// `SpriteDatabase::add_instance` behaves today — a sprite not re-added disappears.
    fn add_instance(&mut self, aii: &InstanceInfo);

    /// Upload a PNG by path and return its assigned `texture_id`.
    ///
    /// Fallibility is asymmetric between backends: `SpriteDatabase` genuinely fails once
    /// `max_textures` is reached, while the dev-mode proxy is fire-and-forget and can only ever
    /// return `Ok` — a real dev-mode failure surfaces player-side, after this call returned, and
    /// is routed through the crash path instead of back through this `Result`.
    fn upload_texture(&mut self, texture_file: &str) -> Result<usize, CanvasError>;

    /// Register a font by path and return its assigned `font_id`.
    ///
    /// Mirrors `upload_texture`: IDs are cumulative and assigned in call order, and the dev-mode
    /// proxy is fire-and-forget (always `Ok`, a real failure surfaces player-side through the crash
    /// path). The player loads the font when it replays the recorded `font_loads`.
    fn load_font(&mut self, path: &str) -> Result<usize, CanvasError>;

    /// Queue one shaped text run for this frame. Occupancy resets after every draw, exactly like
    /// `add_instance` — a run not re-added disappears.
    fn add_text_run(&mut self, run: &TextRun);

    /// The `CreateInfo` the canvas was built with. By value, matching
    /// `SpriteDatabase::get_create_info` (`CreateInfo` is `Copy`); the dev proxy answers from its
    /// cached copy with no round trip.
    fn get_create_info(&self) -> CreateInfo;
}

// Compile-time proof that `SpriteCanvas` stays object safe: `&mut dyn SpriteCanvas` is what
// `SurgeApp` will take, and Rust only checks object safety where the `dyn` type is named.
const _: Option<&mut dyn SpriteCanvas> = None;
