//! Mirrors `InstanceInfo`'s round-trip test (sprite.rs) for the text wire types: a `TextRun`
//! survives a `postcard` encode/decode unchanged.

use surge_sdk::nalgebra::Vector4;
use surge_sdk::{PositionedGlyph, TextRun};

#[test]
fn text_run_round_trips_through_postcard() {
    let original = TextRun {
        font_id: 3,
        px_size: 32.0,
        color: Vector4::new(1.0, 0.5, 0.25, 1.0),
        z: 0.75,
        glyphs: vec![
            PositionedGlyph {
                glyph_id: 36,
                x: 0.0,
                y: 12.0,
            },
            PositionedGlyph {
                glyph_id: 859,
                x: 18.5,
                y: 12.0,
            },
        ],
    };

    let bytes = postcard::to_stdvec(&original).unwrap();
    let decoded: TextRun = postcard::from_bytes(&bytes).unwrap();

    assert_eq!(decoded, original);
}
