use serde::{Deserialize, Serialize};

/// One glyph placed on the baseline by the shaper, in design-resolution pixels (y-down).
///
/// `x`/`y` are the pen position on the baseline; the player adds the rasterization bearing (known
/// only after rasterizing) to derive the quad's top-left corner.
#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub struct PositionedGlyph {
    pub glyph_id: u32,
    pub x: f32,
    pub y: f32,
}

/// A single shaped run: one font at one pixel size, one color and z, with its glyphs already
/// positioned on the baseline. Emitted by `draw_text` and replayed into the player's font database.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct TextRun {
    pub font_id: usize,
    pub px_size: f32,
    pub color: nalgebra::Vector4<f32>,
    pub z: f32,
    pub glyphs: Vec<PositionedGlyph>,
}
