//! A shelf (skyline-lite) allocator for the glyph atlas: pure CPU bookkeeping, no GPU. Cells are
//! packed left to right on a shelf; when a cell does not fit, a new shelf opens below the tallest
//! cell of the current one. Each cell is surrounded by a transparent gutter of at least `gutter`
//! texels on every side so that bilinear sampling of a GPU-scaled glyph never bleeds into a
//! neighbour (the sampled UVs are additionally inset by half a texel when drawing).
//!
/// A packed cell's content rectangle in atlas texels (the gutter is outside it).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(super) struct AtlasRect {
    pub x: u32,
    pub y: u32,
    pub width: u32,
    pub height: u32,
}

pub(super) struct ShelfAllocator {
    width: u32,
    height: u32,
    gutter: u32,
    /// Top edge (y) of the current, open shelf.
    shelf_y: u32,
    /// Next free x on the current shelf (left edge of the next cell's content).
    pen_x: u32,
    /// Tallest content placed on the current shelf so far.
    shelf_height: u32,
}

impl ShelfAllocator {
    /// A fresh allocator over a `width` x `height` atlas, reserving a `gutter`-texel transparent
    /// margin around every cell. The first cell's content starts at `(gutter, gutter)`.
    pub(super) fn new(width: u32, height: u32, gutter: u32) -> Self {
        Self {
            width,
            height,
            gutter,
            shelf_y: gutter,
            pen_x: gutter,
            shelf_height: 0,
        }
    }

    /// Reserves a cell holding `width` x `height` texels of content and returns its content rect,
    /// or `None` if it cannot fit in the remaining atlas (overflow). A placed cell always has at
    /// least `gutter` texels of free space to its left/top (from the starting offset and the
    /// previous cell's trailing gutter) and to its right/bottom (reserved here), so neighbours are
    /// never adjacent.
    pub(super) fn allocate(&mut self, width: u32, height: u32) -> Option<AtlasRect> {
        // A cell with its surrounding gutter must fit within the atlas at all; reject otherwise so
        // the overflow path below is only about running out of vertical room.
        let needed_w = width.checked_add(2 * self.gutter)?;
        let needed_h = height.checked_add(2 * self.gutter)?;
        if needed_w > self.width || needed_h > self.height {
            return None;
        }

        // If the content plus its trailing gutter runs past the right edge, wrap to a new shelf.
        if self.pen_x + width + self.gutter > self.width {
            self.shelf_y += self.shelf_height + self.gutter;
            self.pen_x = self.gutter;
            self.shelf_height = 0;
        }

        // Vertical overflow: the content plus its trailing gutter runs past the bottom edge.
        if self.shelf_y + height + self.gutter > self.height {
            return None;
        }

        let rect = AtlasRect {
            x: self.pen_x,
            y: self.shelf_y,
            width,
            height,
        };

        self.pen_x += width + self.gutter;
        self.shelf_height = self.shelf_height.max(height);

        Some(rect)
    }
}

#[cfg(test)]
#[path = "atlas_allocator_tests.rs"]
mod tests;
