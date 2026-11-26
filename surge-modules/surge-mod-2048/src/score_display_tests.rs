use super::*;
use surge_sdk::{
    BlendingMode, CanvasError, CreateInfo, InstanceInfo, TextRun, TextureFilteringLevel,
    TextureFilteringMode,
};

/// The two boxes `board.png` paints, in board pixels, from `assets/svg/board.svg`.
const SCORE_BOX_X: (f32, f32) = (358.0, 422.0);
const BEST_BOX_X: (f32, f32) = (432.0, 496.0);
const BOX_Y: (f32, f32) = (4.0, 58.0);

fn test_font() -> Font {
    // `cargo test` runs from the crate directory, where `resolve_asset_path`'s repo-root-relative
    // fallback does not apply, so look next to the manifest instead.
    let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("assets")
        .join(FONT_ASSET);
    let bytes = std::fs::read(&path).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    Font::from_bytes(bytes, 0)
        .unwrap_or_else(|e| panic!("{} did not parse ({e}); a git-lfs pointer?", path.display()))
}

#[test]
fn a_run_that_already_fits_keeps_its_size() {
    assert_eq!(fit_px_size(28.0, 40.0, 56.0), 28.0);
    assert_eq!(fit_px_size(28.0, 56.0, 56.0), 28.0);
    // An empty run has nothing to shrink.
    assert_eq!(fit_px_size(28.0, 0.0, 56.0), 28.0);
}

#[test]
fn an_oversized_run_shrinks_to_whole_pixels_and_never_below_one() {
    // 28 * 56 / 112 = 14 exactly.
    assert_eq!(fit_px_size(28.0, 112.0, 56.0), 14.0);
    // 28 * 56 / 90 = 17.42, floored so the shrunk run is certain to fit.
    assert_eq!(fit_px_size(28.0, 90.0, 56.0), 17.0);
    assert_eq!(fit_px_size(28.0, 10_000.0, 56.0), 1.0);
}

#[test]
fn every_number_fits_inside_its_box() {
    let font = test_font();
    for value in [0, 7, 88, 999, 2048, 20_000, 888_888, u32::MAX] {
        let shaped = shape_number(&font, value);
        assert!(
            shaped.width() <= MAX_NUMBER_WIDTH,
            "{value} shaped to {} px wide",
            shaped.width()
        );
        assert!(shaped.px_size() >= 1.0 && shaped.px_size() <= BASE_PX);
        assert_eq!(shaped.len(), value.to_string().len());
    }
}

#[test]
fn short_numbers_are_drawn_at_the_base_size() {
    let font = test_font();
    for value in [0, 7, 88, 999] {
        assert_eq!(shape_number(&font, value).px_size(), BASE_PX);
    }
}

#[test]
fn the_numbers_stay_within_the_boxes_the_board_paints() {
    for (center, (left, right)) in [(SCORE_CENTER_X, SCORE_BOX_X), (BEST_CENTER_X, BEST_BOX_X)] {
        assert_eq!(center, (left + right) / 2.0);
        assert!(center - MAX_NUMBER_WIDTH / 2.0 >= left);
        assert!(center + MAX_NUMBER_WIDTH / 2.0 <= right);
    }

    // Vertically the baseline leaves room for the font's full ascent and descent inside the box,
    // so no digit can spill over the caption above it or the box edge below.
    let metrics = test_font().line_metrics(BASE_PX);
    assert!(BASELINE_Y - metrics.ascent >= BOX_Y.0);
    assert!(BASELINE_Y + metrics.descent <= BOX_Y.1);
}

#[test]
fn the_numbers_draw_in_front_of_the_board() {
    // Larger z draws behind (depth test is GREATER_OR_EQUAL).
    assert!(Z < crate::board_geometry::BoardGeometry::new().get_board_z());
}

/// Records the text runs a `ScoreDisplay` emits; ignores everything else on the canvas.
#[derive(Default)]
struct RecordingCanvas {
    runs: Vec<TextRun>,
}

impl SpriteCanvas for RecordingCanvas {
    fn add_instance(&mut self, _: &InstanceInfo) {}

    fn upload_texture(&mut self, _: &str) -> Result<usize, CanvasError> {
        Ok(0)
    }

    fn load_font(&mut self, _: &str) -> Result<usize, CanvasError> {
        Ok(0)
    }

    fn add_text_run(&mut self, run: &TextRun) {
        self.runs.push(run.clone());
    }

    fn get_create_info(&self) -> CreateInfo {
        CreateInfo {
            blending_mode: BlendingMode::Alpha,
            texture_filtering_mode: TextureFilteringMode::Linear,
            texture_filtering_level: TextureFilteringLevel::X0,
            max_sprites: 64,
            max_textures: 16,
            window_width: 500.0,
            window_height: 800.0,
        }
    }
}

fn loaded_display() -> ScoreDisplay {
    ScoreDisplay {
        font: Some(test_font()),
    }
}

#[test]
fn a_display_without_a_font_draws_nothing() {
    let mut canvas = RecordingCanvas::default();
    ScoreDisplay::default().draw(&mut canvas, 128, 2048, Vec4::from_element(1.0));
    assert!(canvas.runs.is_empty());
}

#[test]
fn drawing_emits_one_run_per_box_centered_on_it() {
    let font = test_font();
    let mut canvas = RecordingCanvas::default();
    loaded_display().draw(&mut canvas, 128, 20_000, Vec4::from_element(1.0));

    assert_eq!(canvas.runs.len(), 2);
    for (run, (value, center)) in canvas
        .runs
        .iter()
        .zip([(128u32, SCORE_CENTER_X), (20_000, BEST_CENTER_X)])
    {
        let shaped = shape_number(&font, value);
        assert_eq!(run.px_size, shaped.px_size());
        assert_eq!(run.glyphs.len(), shaped.len());

        // The run's box straddles the box center, and every glyph sits on the shared baseline.
        let left = run.glyphs[0].x;
        assert!((left + shaped.width() / 2.0 - center).abs() < 0.01);
        assert!(run.glyphs.iter().all(|g| g.y == BASELINE_Y));
    }

    // The long number was the one that had to shrink.
    assert_eq!(canvas.runs[0].px_size, BASE_PX);
    assert!(canvas.runs[1].px_size < BASE_PX);
}

#[test]
fn the_game_over_tint_dims_the_numbers() {
    let mut canvas = RecordingCanvas::default();
    loaded_display().draw(&mut canvas, 128, 2048, Vec4::new(0.35, 0.35, 0.35, 1.0));

    for run in &canvas.runs {
        assert_eq!(run.color, Vec4::new(0.35, 0.35, 0.35, 1.0));
    }
}
