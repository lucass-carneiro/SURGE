//! Module-side text shaping: turns a string + font + pixel size into a positioned glyph run the
//! module can measure and lay out, then emits it into the canvas as a [`TextRun`].
//!
//! Shaping runs here (not in the player) because the dev-mode wire is one-way per turn, so a module
//! cannot query the renderer for metrics mid-turn; it holds the font and measures locally. HarfRust
//! does the shaping; the player rasterizes the glyph ids on its side. The single place that converts
//! font-design conventions (y-up, units-per-em) to the engine's (y-down, design-resolution pixels)
//! is here.

use harfrust::{Buffer, Font as HarfRustFont, ShapeOptions, ShaperFont};
use nalgebra::{Vector2, Vector4};

use crate::canvas::{CanvasError, SpriteCanvas};
use crate::text::{PositionedGlyph, TextRun};

/// Vertical line metrics at one pixel size, in design pixels (y-down).
///
/// `ascent`/`descent` are magnitudes: `ascent` is how far the text reaches above the baseline and
/// `descent` how far below, both non-negative.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct LineMetrics {
    pub ascent: f32,
    pub descent: f32,
    pub line_gap: f32,
}

impl LineMetrics {
    /// Baseline-to-baseline distance for stacked lines: `ascent + descent + line_gap`.
    pub fn line_height(&self) -> f32 {
        self.ascent + self.descent + self.line_gap
    }
}

/// Where `origin` sits on a shaped run's box when it is drawn.
///
/// Horizontal is left / center / right; vertical is top / baseline / bottom. `Center` is the common
/// horizontal-center + baseline case.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TextAnchor {
    TopLeft,
    TopCenter,
    TopRight,
    BaselineLeft,
    Center,
    BaselineRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
}

impl TextAnchor {
    /// Horizontal shift applied to `origin.x` so the run's box aligns as this anchor names, given a
    /// run `width`: left keeps the origin at the box's left edge, center at its middle, right at its
    /// right edge.
    fn horizontal_offset(self, width: f32) -> f32 {
        use TextAnchor::*;
        match self {
            TopLeft | BaselineLeft | BottomLeft => 0.0,
            TopCenter | Center | BottomCenter => -width / 2.0,
            TopRight | BaselineRight | BottomRight => -width,
        }
    }

    /// Vertical shift applied to `origin.y` (y-down) so the baseline lands where this anchor names:
    /// top puts `origin` at the box top (baseline is `ascent` below it), baseline puts it on the
    /// baseline, bottom puts it at the box bottom (baseline is `descent` above it).
    fn vertical_offset(self, ascent: f32, descent: f32) -> f32 {
        use TextAnchor::*;
        match self {
            TopLeft | TopCenter | TopRight => ascent,
            BaselineLeft | Center | BaselineRight => 0.0,
            BottomLeft | BottomCenter | BottomRight => -descent,
        }
    }
}

/// A single shaped run: glyphs positioned on the baseline in design pixels (y-down, baseline at
/// local y = 0, first pen at local x = 0), plus the metrics a module uses to lay out around it.
///
/// Positions are *local*: [`TextCanvasExt::draw_text`] adds the anchor-resolved origin to produce
/// the wire [`TextRun`].
#[derive(Debug, Clone, PartialEq)]
pub struct ShapedText {
    glyphs: Vec<PositionedGlyph>,
    width: f32,
    metrics: LineMetrics,
    px_size: f32,
    font_id: usize,
}

impl ShapedText {
    /// Total advance width of the run, in design pixels.
    pub fn width(&self) -> f32 {
        self.width
    }

    /// Height above the baseline, in design pixels.
    pub fn ascent(&self) -> f32 {
        self.metrics.ascent
    }

    /// Depth below the baseline, in design pixels (a magnitude).
    pub fn descent(&self) -> f32 {
        self.metrics.descent
    }

    /// Extra space between stacked lines, in design pixels.
    pub fn line_gap(&self) -> f32 {
        self.metrics.line_gap
    }

    /// Baseline-to-baseline distance, `ascent + descent + line_gap`.
    pub fn line_height(&self) -> f32 {
        self.metrics.line_height()
    }

    /// The vertical metrics as one value.
    pub fn metrics(&self) -> LineMetrics {
        self.metrics
    }

    /// The pixel size this run was shaped at.
    pub fn px_size(&self) -> f32 {
        self.px_size
    }

    /// The font this run was shaped with.
    pub fn font_id(&self) -> usize {
        self.font_id
    }

    /// The glyphs with their local baseline positions.
    pub fn glyphs(&self) -> &[PositionedGlyph] {
        &self.glyphs
    }

    /// Number of shaped glyphs.
    pub fn len(&self) -> usize {
        self.glyphs.len()
    }

    /// Whether the run shaped to no glyphs (e.g. an empty string).
    pub fn is_empty(&self) -> bool {
        self.glyphs.is_empty()
    }
}

/// A shaping-ready font: a parsed HarfRust face, its scale, its vertical metrics (font units), and
/// the `font_id` the renderer assigned. Owns its byte buffer (through HarfRust's shared blob), so it
/// parses the file exactly once per module process.
pub struct Font {
    font_id: usize,
    face: HarfRustFont,
    units_per_em: f32,
    // Vertical metrics in font units: ascent and descent as non-negative magnitudes, plus line gap.
    ascent_units: f32,
    descent_units: f32,
    line_gap_units: f32,
}

impl std::fmt::Debug for Font {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Font")
            .field("font_id", &self.font_id)
            .field("units_per_em", &self.units_per_em)
            .finish_non_exhaustive()
    }
}

impl Font {
    /// Registers the font with the renderer and parses it for shaping.
    ///
    /// `path` must be openable by this process: pass the path from
    /// [`resolve_asset_path`](crate::resolve_asset_path), exactly as `upload_texture` callers do.
    /// Both sides parse the file — the player loads it when it replays the recorded font load, and
    /// this call reads it here for shaping. Must be called in `on_load` *and*
    /// `on_swapchain_recreate`, like texture uploads, because the player rebuilds its font database
    /// and restarts font ids at 0 on a swapchain recreate.
    pub fn load(canvas: &mut dyn SpriteCanvas, path: &str) -> Result<Font, CanvasError> {
        let font_id = canvas.load_font(path)?;
        let bytes = std::fs::read(path)
            .map_err(|e| CanvasError::new(format!("failed to read font `{path}`: {e}")))?;
        Self::from_bytes(bytes, font_id)
    }

    /// Parses a font from in-memory bytes and tags it with `font_id`.
    ///
    /// The renderer-facing [`load`](Self::load) wraps this after recording the load; this entry
    /// point exists for callers that already hold the bytes (for example embedded via
    /// `include_bytes!`). The bytes are taken by value and kept alive inside the face.
    pub fn from_bytes(bytes: Vec<u8>, font_id: usize) -> Result<Font, CanvasError> {
        let face = HarfRustFont::new(bytes, 0)
            .ok_or_else(|| CanvasError::new("failed to parse font: not a usable TTF/OTF"))?;

        let units_per_em = face.units_per_em();
        if units_per_em == 0 {
            return Err(CanvasError::new("font reports units_per_em = 0"));
        }

        let metrics = face.metrics();
        let (ascent_units, descent_units, line_gap_units) = select_vertical_metrics(
            metrics.use_typo_metrics,
            metrics
                .typo_line
                .map(|lb| (lb.ascender.to_f32(), lb.descender.to_f32(), lb.line_gap.to_f32())),
            metrics
                .hhea_line
                .map(|lb| (lb.ascender.to_f32(), lb.descender.to_f32(), lb.line_gap.to_f32())),
        );

        Ok(Font {
            font_id,
            face,
            units_per_em: units_per_em as f32,
            ascent_units,
            descent_units,
            line_gap_units,
        })
    }

    /// The renderer-assigned font id.
    pub fn id(&self) -> usize {
        self.font_id
    }

    /// Font units per em (the font's design grid resolution).
    pub fn units_per_em(&self) -> f32 {
        self.units_per_em
    }

    /// The font's vertical metrics scaled to `px_size`, without shaping any text.
    pub fn line_metrics(&self, px_size: f32) -> LineMetrics {
        let s = px_per_unit(px_size, self.units_per_em);
        LineMetrics {
            ascent: self.ascent_units * s,
            descent: self.descent_units * s,
            line_gap: self.line_gap_units * s,
        }
    }

    /// Shapes `text` as a single left-to-right run at `px_size`.
    ///
    /// Returns glyphs on the baseline in design pixels (y-down), the run's advance width, and the
    /// font's vertical metrics at this size. HarfRust emits positions in font units; this scales
    /// them by `px_size / units_per_em` (§3.5), accumulating advances along +x and flipping
    /// HarfRust's y-up offsets into the engine's y-down space.
    pub fn shape(&self, text: &str, px_size: f32) -> ShapedText {
        let s = px_per_unit(px_size, self.units_per_em);

        let shaper = ShaperFont::new(&self.face);
        let mut buffer = Buffer::new();
        buffer.push_str(text);
        buffer.guess_segment_properties();
        harfrust::shape(&shaper, &mut buffer, ShapeOptions::new())
            .expect("HarfRust shaping of a fresh buffer cannot fail");

        let infos = buffer.glyph_infos();
        let positions = buffer.glyph_positions();

        let mut glyphs = Vec::with_capacity(infos.len());
        let mut pen_x = 0.0f32;
        for (info, pos) in infos.iter().zip(positions) {
            glyphs.push(PositionedGlyph {
                glyph_id: info.glyph_id,
                x: pen_x + pos.x_offset as f32 * s,
                // HarfRust's y is up; the engine's is down. A positive y_offset lifts the glyph, so
                // it decreases the y-down coordinate.
                y: -(pos.y_offset as f32 * s),
            });
            pen_x += pos.x_advance as f32 * s;
        }

        ShapedText {
            glyphs,
            width: pen_x,
            metrics: self.line_metrics(px_size),
            px_size,
            font_id: self.font_id,
        }
    }
}

/// Draws shaped text, resolving the anchor against the run's measured box so a module never does
/// baseline math itself. A default method over [`SpriteCanvas`] so the anchor math lives in one
/// place and off the trait object.
pub trait TextCanvasExt: SpriteCanvas {
    /// Places `shaped` so `origin` sits where `anchor` names, then emits it as a [`TextRun`] with
    /// the given `color` and `z`.
    fn draw_text(
        &mut self,
        shaped: &ShapedText,
        origin: Vector2<f32>,
        anchor: TextAnchor,
        color: Vector4<f32>,
        z: f32,
    ) {
        let (ox, oy) = anchor_offset(
            anchor,
            shaped.width,
            shaped.metrics.ascent,
            shaped.metrics.descent,
        );
        let glyphs = shaped
            .glyphs
            .iter()
            .map(|g| PositionedGlyph {
                glyph_id: g.glyph_id,
                x: origin.x + ox + g.x,
                y: origin.y + oy + g.y,
            })
            .collect();

        let run = TextRun {
            font_id: shaped.font_id,
            px_size: shaped.px_size,
            color,
            z,
            glyphs,
        };
        self.add_text_run(&run);
    }
}

impl<T: SpriteCanvas + ?Sized> TextCanvasExt for T {}

/// Font-design-units to pixels: `px_size / units_per_em`. Exposed for tests.
#[doc(hidden)]
pub fn px_per_unit(px_size: f32, units_per_em: f32) -> f32 {
    px_size / units_per_em
}

/// Picks the vertical line metrics to use, in the units of the inputs.
///
/// Prefers the `OS/2` typographic line (`typo`) when the font sets `fsSelection.USE_TYPO_METRICS`,
/// else the `hhea` line, falling back to whichever is present. Inputs are `(ascender, descender,
/// line_gap)` with the font-table sign convention (ascender ≥ 0, descender ≤ 0); the returned
/// ascent and descent are non-negative magnitudes. Exposed for tests.
#[doc(hidden)]
pub fn select_vertical_metrics(
    use_typo_metrics: bool,
    typo: Option<(f32, f32, f32)>,
    hhea: Option<(f32, f32, f32)>,
) -> (f32, f32, f32) {
    let chosen = if use_typo_metrics {
        typo.or(hhea)
    } else {
        hhea.or(typo)
    };
    let (ascender, descender, line_gap) = chosen.unwrap_or((0.0, 0.0, 0.0));
    (ascender, -descender, line_gap)
}

/// The `(x, y)` shift added to `origin` so a run of the given `width`, `ascent` and `descent` aligns
/// as `anchor` names. Exposed for tests.
#[doc(hidden)]
pub fn anchor_offset(anchor: TextAnchor, width: f32, ascent: f32, descent: f32) -> (f32, f32) {
    (
        anchor.horizontal_offset(width),
        anchor.vertical_offset(ascent, descent),
    )
}
