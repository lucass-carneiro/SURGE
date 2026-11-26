//! Step 3 shaping-layer tests: font-unit -> px scaling, typo-vs-hhea metric selection, measured
//! width of a known string, and anchor-offset resolution. All pure / GPU-free, over the public API.

use surge_sdk::{
    BlendingMode, CanvasProxy, CreateInfo, Font, SpriteCanvas, TextAnchor, TextCanvasExt,
    TextureFilteringLevel, TextureFilteringMode, anchor_offset, px_per_unit,
    select_vertical_metrics,
};

/// Noto Sans baked into the test binary so the tests do not depend on CWD or asset layout.
const FONT: &[u8] = include_bytes!(concat!(
    env!("CARGO_MANIFEST_DIR"),
    "/../test-assets/fonts/NotoSans-Regular.ttf"
));

/// Openable path to the same font, for the disk-reading `Font::load` path.
const FONT_PATH: &str = concat!(
    env!("CARGO_MANIFEST_DIR"),
    "/../test-assets/fonts/NotoSans-Regular.ttf"
);

fn create_info() -> CreateInfo {
    CreateInfo {
        blending_mode: BlendingMode::Alpha,
        texture_filtering_mode: TextureFilteringMode::Nearest,
        texture_filtering_level: TextureFilteringLevel::X16,
        max_sprites: 32,
        max_textures: 32,
        window_width: 800.0,
        window_height: 600.0,
    }
}

fn load_font() -> Font {
    Font::from_bytes(FONT.to_vec(), 0).expect("Noto Sans parses")
}

// ---- font-unit -> px scaling ----

#[test]
fn px_per_unit_is_the_scale_ratio() {
    assert_eq!(px_per_unit(32.0, 2048.0), 0.015_625);
    assert_eq!(px_per_unit(0.0, 1000.0), 0.0);
}

#[test]
fn width_scales_linearly_with_px_size() {
    let font = load_font();
    let w1 = font.shape("Hello", 20.0).width();
    let w2 = font.shape("Hello", 40.0).width();
    // Positions come from shaping scaled by px/upem, so doubling the size doubles the width.
    assert!((w2 - 2.0 * w1).abs() < 1e-3, "w1={w1} w2={w2}");
}

#[test]
fn metrics_scale_linearly_with_px_size() {
    let font = load_font();
    let m1 = font.line_metrics(16.0);
    let m2 = font.line_metrics(32.0);
    assert!((m2.ascent - 2.0 * m1.ascent).abs() < 1e-3);
    assert!((m2.descent - 2.0 * m1.descent).abs() < 1e-3);
    assert!((m2.line_gap - 2.0 * m1.line_gap).abs() < 1e-3);
}

// ---- metric selection (typo vs hhea) ----

#[test]
fn selects_typo_when_the_font_requests_it() {
    // typo and hhea both present; USE_TYPO_METRICS set -> typo wins. Descender sign is normalized
    // to a non-negative magnitude.
    let (asc, desc, gap) = select_vertical_metrics(
        true,
        Some((800.0, -200.0, 90.0)),
        Some((750.0, -250.0, 0.0)),
    );
    assert_eq!((asc, desc, gap), (800.0, 200.0, 90.0));
}

#[test]
fn selects_hhea_when_typo_is_not_requested() {
    let (asc, desc, gap) = select_vertical_metrics(
        false,
        Some((800.0, -200.0, 90.0)),
        Some((750.0, -250.0, 10.0)),
    );
    assert_eq!((asc, desc, gap), (750.0, 250.0, 10.0));
}

#[test]
fn falls_back_to_the_other_line_when_the_preferred_is_absent() {
    // Asks for typo but the font has no OS/2 typo line: fall back to hhea.
    let got = select_vertical_metrics(true, None, Some((750.0, -250.0, 10.0)));
    assert_eq!(got, (750.0, 250.0, 10.0));
    // Neither present: zeros, not a panic.
    assert_eq!(select_vertical_metrics(false, None, None), (0.0, 0.0, 0.0));
}

#[test]
fn real_font_has_plausible_vertical_metrics() {
    let font = load_font();
    let m = font.line_metrics(32.0);
    assert!(m.ascent > 0.0 && m.descent > 0.0, "{m:?}");
    // A line's ascent+descent is close to the em at a typical pixel size, and the full line height
    // (with gap) is at least that.
    assert!(m.ascent < 48.0 && m.descent < 32.0, "{m:?}");
    assert!(m.line_height() >= m.ascent + m.descent);
}

// ---- measured width of a known string ----

#[test]
fn shapes_ascii_to_one_glyph_per_char_with_increasing_pen() {
    let font = load_font();
    let shaped = font.shape("Hello", 32.0);
    assert_eq!(shaped.len(), 5, "plain ASCII has no ligatures in Noto Sans");

    let xs: Vec<f32> = shaped.glyphs().iter().map(|g| g.x).collect();
    assert_eq!(xs[0], 0.0, "first pen sits at local x = 0");
    for pair in xs.windows(2) {
        assert!(pair[1] > pair[0], "pen must advance: {xs:?}");
    }
    // Every glyph is on the baseline for a single LTR line.
    assert!(shaped.glyphs().iter().all(|g| g.y == 0.0));

    // Loose but meaningful: five Latin glyphs at 32px span a few multiples of the size.
    assert!(
        shaped.width() > 32.0 && shaped.width() < 32.0 * 6.0,
        "width {} implausible for \"Hello\" at 32px",
        shaped.width()
    );
}

#[test]
fn empty_string_shapes_to_nothing() {
    let font = load_font();
    let shaped = font.shape("", 32.0);
    assert!(shaped.is_empty());
    assert_eq!(shaped.width(), 0.0);
    // Metrics are still the font's, independent of the (absent) text.
    assert!(shaped.ascent() > 0.0);
}

// ---- anchor-offset resolution ----

#[test]
fn anchor_offsets_resolve_against_the_box() {
    let (w, asc, desc) = (100.0, 24.0, 8.0);

    // Horizontal: left keeps the origin, center shifts by -W/2, right by -W.
    assert_eq!(anchor_offset(TextAnchor::BaselineLeft, w, asc, desc).0, 0.0);
    assert_eq!(anchor_offset(TextAnchor::Center, w, asc, desc).0, -50.0);
    assert_eq!(
        anchor_offset(TextAnchor::BaselineRight, w, asc, desc).0,
        -100.0
    );

    // Vertical (y-down): top drops the baseline by +ascent, baseline is 0, bottom lifts by -descent.
    assert_eq!(anchor_offset(TextAnchor::TopLeft, w, asc, desc).1, 24.0);
    assert_eq!(anchor_offset(TextAnchor::BaselineLeft, w, asc, desc).1, 0.0);
    assert_eq!(anchor_offset(TextAnchor::BottomLeft, w, asc, desc).1, -8.0);
}

#[test]
fn draw_text_emits_a_run_anchored_at_the_origin() {
    let font = load_font();
    let shaped = font.shape("Hello", 32.0);
    let w = shaped.width();
    let first_local_x = shaped.glyphs()[0].x; // 0.0

    let mut proxy = CanvasProxy::new(create_info());
    let origin = surge_sdk::nalgebra::Vector2::new(400.0, 300.0);
    let color = surge_sdk::nalgebra::Vector4::new(1.0, 1.0, 1.0, 1.0);
    proxy.draw_text(&shaped, origin, TextAnchor::Center, color, 0.5);

    let ops = proxy.take_ops();
    assert_eq!(ops.text_runs.len(), 1);
    let run = &ops.text_runs[0];
    assert_eq!(run.font_id, 0);
    assert_eq!(run.px_size, 32.0);
    assert_eq!(run.z, 0.5);
    assert_eq!(run.glyphs.len(), shaped.len());

    // Center anchor + baseline: first glyph lands at origin.x - W/2 (+ its local x), origin.y.
    let g0 = run.glyphs[0];
    assert!(
        (g0.x - (origin.x - w / 2.0 + first_local_x)).abs() < 1e-3,
        "{g0:?}"
    );
    assert_eq!(g0.y, origin.y);
    // Glyph spacing is preserved end to end.
    let last = *run.glyphs.last().unwrap();
    assert!((last.x - g0.x - (shaped.glyphs().last().unwrap().x - first_local_x)).abs() < 1e-3);
}

// ---- Font::load glue (reads disk, records the load, assigns the id) ----

#[test]
fn load_records_the_font_and_assigns_the_proxy_id() {
    let mut proxy = CanvasProxy::new(create_info());
    let font = Font::load(&mut proxy, FONT_PATH).expect("load Noto from disk");
    assert_eq!(font.id(), 0);
    let next = proxy.load_font("another.ttf").unwrap();
    assert_eq!(next, 1, "ids are cumulative in call order");

    let ops = proxy.take_ops();
    assert_eq!(ops.font_loads, [FONT_PATH, "another.ttf"]);
}
