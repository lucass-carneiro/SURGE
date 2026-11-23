use super::*;

const GUTTER: u32 = 1;

#[test]
fn packs_a_shelf_left_to_right_with_a_gutter_between_cells() {
    let mut a = ShelfAllocator::new(128, 128, GUTTER);

    let first = a.allocate(10, 20).unwrap();
    let second = a.allocate(15, 12).unwrap();
    let third = a.allocate(8, 30).unwrap();

    // All three sit on the same (first) shelf.
    assert_eq!(first.y, GUTTER);
    assert_eq!(second.y, GUTTER);
    assert_eq!(third.y, GUTTER);

    // Content starts after the left gutter and advances left to right.
    assert_eq!(first.x, GUTTER);
    assert!(second.x > first.x);
    assert!(third.x > second.x);

    // There is at least a gutter of free space between consecutive cells.
    assert!(second.x >= first.x + first.width + GUTTER);
    assert!(third.x >= second.x + second.width + GUTTER);
}

#[test]
fn wraps_to_the_next_shelf_when_a_cell_does_not_fit_horizontally() {
    // 40-wide atlas: two 15-wide cells fit on a shelf, the third wraps.
    let mut a = ShelfAllocator::new(40, 128, GUTTER);

    let first = a.allocate(15, 20).unwrap();
    let second = a.allocate(15, 10).unwrap();
    let wrapped = a.allocate(15, 8).unwrap();

    assert_eq!(first.y, GUTTER);
    assert_eq!(second.y, GUTTER);

    // The wrapped cell drops to a new shelf below the tallest cell of the first (height 20),
    // separated by a gutter, and restarts at the left gutter.
    assert_eq!(wrapped.x, GUTTER);
    assert!(wrapped.y >= first.y + 20 + GUTTER);
    assert_eq!(wrapped.y, GUTTER + 20 + GUTTER);
}

#[test]
fn reports_overflow_when_vertical_space_is_exhausted() {
    // Tall atlas, 1 wide shelf of room: each 20-tall cell is its own shelf.
    let mut a = ShelfAllocator::new(16, 48, GUTTER);

    assert!(a.allocate(10, 20).is_some()); // shelf at y=1, occupies rows 1..21
    assert!(a.allocate(10, 20).is_some()); // shelf at y=22, occupies rows 22..42
    // Next shelf would start at y=43; 43 + 20 + 1 = 64 > 48 -> overflow.
    assert!(a.allocate(10, 20).is_none());
}

#[test]
fn rejects_a_cell_too_large_for_the_atlas_even_empty() {
    let mut a = ShelfAllocator::new(32, 32, GUTTER);
    // 32 wide + 2*gutter = 34 > 32: never fits.
    assert!(a.allocate(32, 10).is_none());
    assert!(a.allocate(10, 32).is_none());
    // But the largest cell that leaves room for its gutter does fit.
    assert!(a.allocate(30, 30).is_some());
}

#[test]
fn consecutive_shelves_keep_a_vertical_gutter() {
    let mut a = ShelfAllocator::new(20, 128, GUTTER);

    let first = a.allocate(15, 10).unwrap(); // shelf 0
    let second = a.allocate(15, 10).unwrap(); // wraps to shelf 1

    assert_eq!(first.y, GUTTER);
    assert!(second.y >= first.y + first.height + GUTTER);
}
