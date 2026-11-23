//! The skrifa-outline -> zeno-coverage adapter (design §3.2): skrifa draws a glyph outline in y-up
//! font design space; the adapter negates y to hand zeno the engine's y-down space, and zeno's
//! default `Origin::TopLeft` rasterizes it into a single-channel coverage mask. The convention this
//! pins (an above-baseline glyph has a negative `Placement.top`) was established by the Step 0 spike
//! and is relied on by the placement math in [`super::glyph_cache::glyph_quad`].

use skrifa::instance::{LocationRef, Size};
use skrifa::outline::{DrawSettings, OutlinePen};
use skrifa::{FontRef, GlyphId, MetadataProvider};
use zeno::{Command, Mask, Point};

use super::glyph_cache::RasterizedGlyph;

/// Collects a skrifa outline as zeno path commands, negating y to convert y-up font design space
/// into the engine's y-down space.
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

/// Rasterizes one glyph outline into a coverage bitmap at `px` pixels. Returns `None` when the
/// glyph has no visible bitmap — an unknown glyph id, an outline that fails to draw, or an empty
/// outline (e.g. a space) — so the caller caches a draw-nothing entry.
///
/// `bytes` is re-parsed per call: the borrowed `FontRef` cannot be stored alongside the bytes it
/// references, so the player rebuilds it on demand (the parse-once-per-side rule keeps this to one
/// lazy, zero-copy parse, §3.2).
pub(super) fn rasterize_glyph(bytes: &[u8], glyph_id: u32, px: f32) -> Option<RasterizedGlyph> {
    let font = FontRef::new(bytes).ok()?;
    let glyph = font.outline_glyphs().get(GlyphId::new(glyph_id))?;

    let mut pen = ZenoPen::default();
    glyph
        .draw(
            DrawSettings::unhinted(Size::new(px), LocationRef::default()),
            &mut pen,
        )
        .ok()?;

    if pen.cmds.is_empty() {
        return None;
    }

    let (coverage, placement) = Mask::new(pen.cmds.as_slice()).render();
    if placement.width == 0 || placement.height == 0 {
        return None;
    }

    Some(RasterizedGlyph {
        coverage,
        left: placement.left,
        top: placement.top,
        width: placement.width,
        height: placement.height,
    })
}

#[cfg(test)]
#[path = "rasterizer_tests.rs"]
mod tests;
