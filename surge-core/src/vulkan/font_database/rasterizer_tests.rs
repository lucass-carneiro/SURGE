use super::*;

/// The same committed Noto Sans the Step 0 spike uses, baked into the test binary so these tests
/// need neither a GPU nor a module asset layout.
const FONT: &[u8] = include_bytes!("../../../../test-assets/fonts/NotoSans-Regular.ttf");

const PX: f32 = 64.0;

fn glyph_id_for(ch: char) -> u32 {
    FontRef::new(FONT)
        .expect("skrifa parses Noto Sans")
        .charmap()
        .map(ch)
        .unwrap_or_else(|| panic!("no glyph for {ch:?}"))
        .to_u32()
}

#[test]
fn rasterizes_a_visible_glyph_with_coverage_and_sane_bearings() {
    let gid = glyph_id_for('A');
    let glyph = rasterize_glyph(FONT, gid, PX).expect("'A' rasterizes to a bitmap");

    assert!(
        glyph.width > 0 && glyph.height > 0,
        "coverage bitmap is empty: {}x{}",
        glyph.width,
        glyph.height
    );
    assert_eq!(
        glyph.coverage.len(),
        (glyph.width * glyph.height) as usize,
        "coverage buffer must be width*height bytes"
    );
    assert!(
        glyph.coverage.iter().any(|&c| c > 0),
        "coverage mask is entirely zero"
    );

    // 'A' sits above the baseline: in y-down space its top row is above the origin, so the top
    // bearing is negative and its magnitude is on the order of the cap height (below the em size).
    let top_bearing = -glyph.top;
    assert!(
        top_bearing > 0 && (top_bearing as f32) < PX * 1.5,
        "top bearing {top_bearing} implausible at {PX}px (top={})",
        glyph.top
    );
    assert!(
        (glyph.height as f32) <= PX * 1.5,
        "glyph height {} implausible at {PX}px",
        glyph.height
    );
    assert!(
        glyph.left >= 0 && (glyph.left as f32) < PX,
        "left bearing {} implausible at {PX}px",
        glyph.left
    );
}

#[test]
fn a_space_has_no_visible_bitmap() {
    let gid = glyph_id_for(' ');
    assert!(
        rasterize_glyph(FONT, gid, PX).is_none(),
        "a space glyph has an empty outline and must rasterize to nothing"
    );
}

#[test]
fn an_out_of_range_glyph_id_returns_none() {
    // Far beyond any real glyph index in the font: no outline, so no bitmap.
    assert!(rasterize_glyph(FONT, u32::MAX, PX).is_none());
}
