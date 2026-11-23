//! The player-side glyph cache: pure CPU bookkeeping over the shelf atlas allocator, with no GPU
//! dependency so it is unit-testable. A glyph is keyed by `(font_id, glyph_id, px bucket)`; on a
//! miss the caller rasterizes once and the cache either packs the bitmap into the atlas (handing
//! back the cell and coverage to upload) or records a zero-size *sentinel* so a glyph that
//! rasterized empty or overflowed the atlas becomes a cache hit and is never rasterized again.
//!
//! The pen->quad placement math and the atlas-cell UV transform live here too, as pure functions,
//! because getting the y-up<->y-down convention wrong is the main correctness risk (design §3.5).

use nalgebra::{Vector2, Vector4};
use std::collections::HashMap;

use super::atlas_allocator::{AtlasRect, ShelfAllocator};

/// Identifies a cached glyph. `px` is the rounded pixel bucket (see [`px_bucket`]): glyphs shaped
/// at sizes that round to the same integer share one atlas entry.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub(super) struct GlyphKey {
    pub font_id: usize,
    pub glyph_id: u32,
    pub px: u32,
}

/// A cached glyph's atlas cell plus the bearings needed to place its quad. A zero-size `cell`
/// marks a glyph that draws nothing — either it rasterized empty (e.g. a space) or the atlas
/// overflowed and a sentinel was cached.
#[derive(Debug, Clone, Copy, PartialEq)]
pub(super) struct GlyphEntry {
    pub cell: AtlasRect,
    /// Left bearing (zeno `Placement.left`): x offset from the pen to the bitmap's left edge.
    pub left: i32,
    /// Signed baseline->top-row offset (zeno `Placement.top`), negative for a glyph above the
    /// baseline, so the quad's top-left y is `pen_y + top` (§3.5).
    pub top: i32,
}

impl GlyphEntry {
    /// A draw-nothing entry (empty rasterization or atlas-overflow sentinel).
    fn sentinel() -> Self {
        Self {
            cell: AtlasRect {
                x: 0,
                y: 0,
                width: 0,
                height: 0,
            },
            left: 0,
            top: 0,
        }
    }

    pub(super) fn draws_nothing(&self) -> bool {
        self.cell.width == 0 || self.cell.height == 0
    }
}

/// A rasterized glyph bitmap: the coverage mask plus the size and bearings zeno reports.
pub(super) struct RasterizedGlyph {
    pub coverage: Vec<u8>,
    pub left: i32,
    pub top: i32,
    pub width: u32,
    pub height: u32,
}

/// The glyph cache: a map from [`GlyphKey`] to [`GlyphEntry`] backed by the shelf atlas allocator.
pub(super) struct GlyphCache {
    entries: HashMap<GlyphKey, GlyphEntry>,
    allocator: ShelfAllocator,
}

impl GlyphCache {
    pub(super) fn new(allocator: ShelfAllocator) -> Self {
        Self {
            entries: HashMap::new(),
            allocator,
        }
    }

    /// Looks up `key`; on a miss, calls `rasterize` exactly once and either packs the returned
    /// bitmap into the atlas or caches a sentinel (empty rasterization or atlas overflow). Returns
    /// the resolved entry and, when a new cell was packed, the `(cell, coverage)` the caller must
    /// upload to the atlas. On a cache hit `rasterize` is never called and no upload is returned.
    pub(super) fn get_or_insert(
        &mut self,
        key: GlyphKey,
        rasterize: impl FnOnce() -> Option<RasterizedGlyph>,
    ) -> (GlyphEntry, Option<(AtlasRect, Vec<u8>)>) {
        if let Some(entry) = self.entries.get(&key) {
            return (*entry, None);
        }

        let (entry, upload) = match rasterize() {
            None => (GlyphEntry::sentinel(), None),
            Some(glyph) => match self.allocator.allocate(glyph.width, glyph.height) {
                None => {
                    log::warn!(
                        "glyph atlas overflow: dropping glyph {} (font {}, {}px); it will draw nothing",
                        key.glyph_id,
                        key.font_id,
                        key.px
                    );
                    (GlyphEntry::sentinel(), None)
                }
                Some(cell) => (
                    GlyphEntry {
                        cell,
                        left: glyph.left,
                        top: glyph.top,
                    },
                    Some((cell, glyph.coverage)),
                ),
            },
        };

        self.entries.insert(key, entry);
        (entry, upload)
    }

    /// Overwrites `key`'s entry with a draw-nothing sentinel. Used when a glyph was packed but its
    /// GPU upload could not be staged, so the already-reserved (never-written) cell is never
    /// sampled and the glyph is not re-rasterized.
    pub(super) fn mark_dropped(&mut self, key: GlyphKey) {
        self.entries.insert(key, GlyphEntry::sentinel());
    }
}

/// Rounds a continuous pixel size to the integer atlas bucket, clamped to at least 1 so a
/// degenerate size never asks the rasterizer for a zero-pixel glyph (§3.5, §6.3).
pub(super) fn px_bucket(px_size: f32) -> u32 {
    px_size.round().max(1.0) as u32
}

/// Top-left corner (design px) and size of a glyph quad, from the baseline pen position and the
/// cached bearings. The pen is snapped to integer design-px first (v1 snapping, §3.5); the quad's
/// size is the bitmap size held in the atlas cell.
pub(super) fn glyph_quad(pen_x: f32, pen_y: f32, entry: &GlyphEntry) -> (Vector2<f32>, Vector2<f32>) {
    let position = Vector2::new(pen_x.round() + entry.left as f32, pen_y.round() + entry.top as f32);
    let scale = Vector2::new(entry.cell.width as f32, entry.cell.height as f32);
    (position, scale)
}

/// Atlas-cell UV transform `(scale.xy, offset.xy)` for the font vertex shader, inset by half a
/// texel on every side so bilinear sampling of a GPU-scaled glyph stays strictly within the cell
/// and never reads the surrounding gutter (§3.2). Only meaningful for a non-empty cell.
pub(super) fn glyph_cell_uv(cell: AtlasRect, atlas_size: u32) -> Vector4<f32> {
    let atlas = atlas_size as f32;
    Vector4::new(
        (cell.width as f32 - 1.0) / atlas,
        (cell.height as f32 - 1.0) / atlas,
        (cell.x as f32 + 0.5) / atlas,
        (cell.y as f32 + 0.5) / atlas,
    )
}

#[cfg(test)]
#[path = "glyph_cache_tests.rs"]
mod tests;
