//! The SCORE and BEST numbers drawn into the two boxes `board.png` already paints at the top of
//! the board.

use surge_sdk::{
    Font, ShapedText, SpriteCanvas, TextAnchor, TextCanvasExt,
    nalgebra::{Vector2 as Vec2, Vector4 as Vec4},
};

use crate::MODULE_NAME;

const FONT_ASSET: &str = "fonts/ClearSans-Bold.ttf";

/// Horizontal centers of the two boxes. `board.svg`'s `score_display` and `top_score_display`
/// rects span x 358..422 and x 432..496 on the 500x800 board.
const SCORE_CENTER_X: f32 = 390.0;
const BEST_CENTER_X: f32 = 464.0;

/// Widest a number may be drawn: the 64 px box less 4 px of padding on each side.
const MAX_NUMBER_WIDTH: f32 = 56.0;

/// Baseline shared by both numbers, placed so the digits sit between the "SCORE" / "BEST" captions
/// baked into `board.png` (their baseline is at y ≈ 18) and the bottom of the boxes at y = 58.
const BASELINE_Y: f32 = 47.0;

/// Size the numbers are shaped at before any shrink-to-fit.
const BASE_PX: f32 = 28.0;

/// In front of the board (larger z draws behind).
const Z: f32 = 0.0;

/// White, as in the original game; multiplied by the board tint so the numbers dim with everything
/// else on a game over.
const COLOR: Vec4<f32> = Vec4::new(1.0, 1.0, 1.0, 1.0);

#[derive(Debug, Default)]
pub(crate) struct ScoreDisplay {
    /// `None` before the first load, and whenever the font failed to load. Re-registered on every
    /// load because the player restarts font ids at 0 when it rebuilds its font database.
    font: Option<Font>,
}

impl ScoreDisplay {
    /// Registers the font with the renderer and parses it for shaping. Must run in `on_load` *and*
    /// `on_swapchain_recreate`, like the texture uploads.
    pub(crate) fn load_font(&mut self, canvas: &mut dyn SpriteCanvas) {
        let path = surge_sdk::resolve_asset_path(MODULE_NAME, FONT_ASSET);
        self.font = match Font::load(canvas, path.to_str().unwrap()) {
            Ok(font) => Some(font),
            Err(e) => {
                log::error!("failed to load font `{}`: {e}", path.display());
                None
            }
        };
    }

    pub(crate) fn draw(
        &self,
        canvas: &mut dyn SpriteCanvas,
        score: u32,
        best: u32,
        tint: Vec4<f32>,
    ) {
        let Some(font) = self.font.as_ref() else {
            return;
        };

        let color = COLOR.component_mul(&tint);
        draw_number(canvas, font, SCORE_CENTER_X, score, color);
        draw_number(canvas, font, BEST_CENTER_X, best, color);
    }
}

fn draw_number(
    canvas: &mut dyn SpriteCanvas,
    font: &Font,
    center_x: f32,
    value: u32,
    color: Vec4<f32>,
) {
    canvas.draw_text(
        &shape_number(font, value),
        Vec2::new(center_x, BASELINE_Y),
        TextAnchor::Center,
        color,
        Z,
    );
}

/// Shapes `value`'s digits at [`BASE_PX`], dropping to a smaller size when they are too wide
/// for the box.
fn shape_number(font: &Font, value: u32) -> ShapedText {
    let text = value.to_string();
    let shaped = font.shape(&text, BASE_PX);

    let fitted = fit_px_size(BASE_PX, shaped.width(), MAX_NUMBER_WIDTH);
    if fitted == BASE_PX {
        shaped
    } else {
        font.shape(&text, fitted)
    }
}

/// Pixel size at which a run measuring `width` at `px` fits inside `max_width`, never growing it.
///
/// The result is a whole number of pixels so the shaped size matches the size the player
/// rasterizes at, which buckets to the nearest integer.
fn fit_px_size(px: f32, width: f32, max_width: f32) -> f32 {
    if width <= max_width || width <= 0.0 {
        px
    } else {
        (px * max_width / width).floor().max(1.0)
    }
}

#[cfg(test)]
#[path = "score_display_tests.rs"]
mod tests;
