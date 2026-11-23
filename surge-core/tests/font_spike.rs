//! Step 0 dependency spike (throwaway).
//!
//! Proves the three external crates before any GPU code depends on them:
//!   - harfrust  : shapes a string into positioned glyphs (font units),
//!   - skrifa    : draws a glyph outline (y-up font design space),
//!   - zeno      : rasterizes that outline into a grayscale coverage mask.
//!
//! It isolates the two things most likely to be wrong later: the font-unit ->
//! pixel scaling on the shaping side, and the y-up <-> y-down convention on the
//! raster side. Both are asserted here so a regression shows up without a GPU.
//!
//! Finding pinned by this spike: skrifa emits outlines in y-up font space
//! (baseline at 0, up is +y). The adapter negates y so zeno rasterizes in the
//! engine's y-down space. With zeno's default `Origin::TopLeft`, an above-the-
//! baseline glyph then has a negative `Placement.top`: it is the signed offset
//! from the baseline to the bitmap's top row (up is negative), so the quad's
//! top-left y is `baseline_y + placement.top` and the baseline-to-top bearing is
//! `-placement.top`. `Placement.left` is the left bearing. This matches the plan
//! (§3.2 "flipping to y-down") and is the convention Step 5 should adopt.

use harfrust::{Buffer, Font as HrFont, ShapeOptions, ShaperFont};
use skrifa::instance::{LocationRef, Size};
use skrifa::outline::{DrawSettings, OutlinePen};
use skrifa::{FontRef, GlyphId, MetadataProvider};
use zeno::{Command, Mask, Placement, Point};

/// One committed copy of Noto Sans, baked into the test binary so the spike does
/// not depend on CWD or a module's asset layout.
const FONT: &[u8] = include_bytes!("../../test-assets/fonts/NotoSans-Regular.ttf");

/// Arbitrary pixel size for the spike; large enough that a glyph rasterizes to a
/// comfortably non-trivial bitmap.
const PX: f32 = 64.0;

/// skrifa `OutlinePen` -> zeno `Command` adapter. skrifa emits coordinates in
/// y-up font design space (baseline at 0, up is +y); the adapter negates y to
/// hand zeno the engine's y-down space, then renders with zeno's default
/// `Origin::TopLeft` (see module docs).
#[derive(Default)]
struct ZenoPen {
    cmds: Vec<Command>,
}

impl OutlinePen for ZenoPen {
    fn move_to(&mut self, x: f32, y: f32) {
        self.cmds.push(Command::MoveTo(Point::new(x, -y)));
    }
    fn line_to(&mut self, x: f32, y: f32) {
        self.cmds.push(Command::LineTo(Point::new(x, -y)));
    }
    fn quad_to(&mut self, cx: f32, cy: f32, x: f32, y: f32) {
        self.cmds
            .push(Command::QuadTo(Point::new(cx, -cy), Point::new(x, -y)));
    }
    fn curve_to(&mut self, cx0: f32, cy0: f32, cx1: f32, cy1: f32, x: f32, y: f32) {
        self.cmds.push(Command::CurveTo(
            Point::new(cx0, -cy0),
            Point::new(cx1, -cy1),
            Point::new(x, -y),
        ));
    }
    fn close(&mut self) {
        self.cmds.push(Command::Close);
    }
}

/// Shapes `text` with harfrust and returns the shaped glyph ids and their pen-x
/// positions in pixels, plus the total advance width in pixels.
fn shape_px(text: &str) -> (Vec<u32>, Vec<f32>, f32) {
    let font = HrFont::new(FONT, 0).expect("harfrust parse");
    let upem = font.units_per_em() as f32;
    assert!(upem > 0.0, "units_per_em must be positive");
    let px_per_unit = PX / upem;

    let shaper = ShaperFont::new(&font);
    let mut buffer = Buffer::new();
    buffer.push_str(text);
    buffer.guess_segment_properties(); // sets direction/script/language from the text
    harfrust::shape(&shaper, &mut buffer, ShapeOptions::new()).expect("harfrust shape");

    let infos = buffer.glyph_infos();
    let positions = buffer.glyph_positions();
    assert_eq!(infos.len(), positions.len());

    let mut ids = Vec::new();
    let mut pen_xs = Vec::new();
    let mut pen = 0.0f32;
    for (info, pos) in infos.iter().zip(positions) {
        ids.push(info.glyph_id);
        pen_xs.push(pen + pos.x_offset as f32 * px_per_unit);
        pen += pos.x_advance as f32 * px_per_unit;
    }
    (ids, pen_xs, pen)
}

#[test]
fn harfrust_shapes_two_glyphs_with_increasing_pen_x() {
    let (ids, pen_xs, width) = shape_px("Ag");

    assert_eq!(
        ids.len(),
        2,
        "\"Ag\" should shape to two glyphs in Noto Sans, got {}",
        ids.len()
    );
    assert!(
        pen_xs[1] > pen_xs[0],
        "second glyph pen-x ({}) must advance past the first ({})",
        pen_xs[1],
        pen_xs[0]
    );
    // Loose but meaningful band: two Latin glyphs at 64px are comfortably within
    // a fraction of, and a few multiples of, the pixel size.
    assert!(
        width > PX * 0.3 && width < PX * 4.0,
        "advance width {width}px implausible for \"Ag\" at {PX}px"
    );
}

#[test]
fn skrifa_zeno_rasterizes_glyph_with_sane_bearings() {
    // Use the shaped glyph id for 'A' so the id is resolved through the font's
    // cmap exactly as production will (harfrust and skrifa agree: same file).
    let (ids, _, _) = shape_px("A");
    let gid = ids[0];

    let font = FontRef::new(FONT).expect("skrifa parse");
    let outlines = font.outline_glyphs();
    let glyph = outlines
        .get(GlyphId::new(gid))
        .expect("outline glyph for 'A'");

    let mut pen = ZenoPen::default();
    glyph
        .draw(
            DrawSettings::unhinted(Size::new(PX), LocationRef::default()),
            &mut pen,
        )
        .expect("skrifa draw");
    assert!(
        !pen.cmds.is_empty(),
        "'A' produced no outline commands (empty path)"
    );

    // Default Origin::TopLeft; the adapter already put the outline in y-down.
    let (coverage, placement): (Vec<u8>, Placement) = Mask::new(pen.cmds.as_slice()).render();

    assert!(
        placement.width > 0 && placement.height > 0,
        "coverage bitmap is empty: {}x{}",
        placement.width,
        placement.height
    );
    assert_eq!(
        coverage.len(),
        (placement.width * placement.height) as usize,
        "Alpha coverage buffer must be width*height bytes"
    );
    assert!(
        coverage.iter().any(|&c| c > 0),
        "coverage mask is entirely zero"
    );

    // 'A' sits above the baseline. In y-down space its top row is above the
    // origin, so placement.top is negative and the baseline-to-top bearing is
    // -placement.top, on the order of the cap height (below the em size). The
    // left bearing is a small, non-negative inset.
    let top_bearing = -placement.top;
    assert!(
        top_bearing > 0 && (top_bearing as f32) < PX * 1.5,
        "top bearing {top_bearing} implausible at {PX}px (placement.top={})",
        placement.top
    );
    assert!(
        (placement.height as f32) <= PX * 1.5,
        "glyph height {} implausible at {PX}px",
        placement.height
    );
    assert!(
        placement.left >= 0 && (placement.left as f32) < PX,
        "left bearing {} implausible at {PX}px",
        placement.left
    );

    println!(
        "spike: 'A' @ {PX}px -> {}x{} px, left={} top={} (top_bearing={})",
        placement.width, placement.height, placement.left, placement.top, top_bearing
    );
}
