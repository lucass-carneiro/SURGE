use super::*;

fn raster(width: u32, height: u32) -> RasterizedGlyph {
    RasterizedGlyph {
        coverage: vec![255u8; (width * height) as usize],
        left: 1,
        top: -(height as i32),
        width,
        height,
    }
}

#[test]
fn px_bucket_rounds_to_nearest_and_clamps_to_one() {
    assert_eq!(px_bucket(31.6), 32);
    assert_eq!(px_bucket(32.4), 32);
    assert_eq!(px_bucket(32.5), 33);
    assert_eq!(px_bucket(1.0), 1);
    // Degenerate sizes clamp to 1 rather than asking for a zero-pixel glyph.
    assert_eq!(px_bucket(0.2), 1);
    assert_eq!(px_bucket(0.0), 1);
    assert_eq!(px_bucket(-5.0), 1);
}

#[test]
fn first_reference_rasterizes_and_packs_second_is_a_cache_hit() {
    let mut cache = GlyphCache::new(ShelfAllocator::new(256, 256, 1));
    let key = GlyphKey {
        font_id: 0,
        glyph_id: 42,
        px: 32,
    };

    let mut calls = 0;
    let (e1, up1) = cache.get_or_insert(key, || {
        calls += 1;
        Some(raster(4, 6))
    });
    assert_eq!(calls, 1);
    assert!(!e1.draws_nothing());
    let (cell, coverage) = up1.expect("a freshly packed glyph yields an upload");
    assert_eq!((cell.width, cell.height), (4, 6));
    assert_eq!(coverage.len(), 24);

    // Second reference to the same key must hit the cache and never rasterize again.
    let (e2, up2) = cache.get_or_insert(key, || {
        calls += 1;
        panic!("a cached glyph must not be rasterized again");
    });
    assert_eq!(calls, 1);
    assert_eq!(e2, e1);
    assert!(up2.is_none(), "a cache hit uploads nothing");
}

#[test]
fn sizes_in_the_same_px_bucket_share_an_entry_adjacent_buckets_do_not() {
    let mut cache = GlyphCache::new(ShelfAllocator::new(256, 256, 1));
    let mut calls = 0;

    // 31.6 and 32.4 both round to 32 -> same key.
    let k_a = GlyphKey {
        font_id: 0,
        glyph_id: 7,
        px: px_bucket(31.6),
    };
    let k_b = GlyphKey {
        font_id: 0,
        glyph_id: 7,
        px: px_bucket(32.4),
    };
    assert_eq!(k_a, k_b);

    cache.get_or_insert(k_a, || {
        calls += 1;
        Some(raster(3, 3))
    });
    cache.get_or_insert(k_b, || {
        calls += 1;
        panic!("same px bucket must hit the cache");
    });
    assert_eq!(calls, 1);

    // 33.0 rounds to 33 -> distinct key, distinct rasterization.
    let k_c = GlyphKey {
        font_id: 0,
        glyph_id: 7,
        px: px_bucket(33.0),
    };
    assert_ne!(k_a, k_c);
    cache.get_or_insert(k_c, || {
        calls += 1;
        Some(raster(3, 3))
    });
    assert_eq!(calls, 2);
}

#[test]
fn an_empty_rasterization_caches_a_draw_nothing_entry() {
    let mut cache = GlyphCache::new(ShelfAllocator::new(256, 256, 1));
    let key = GlyphKey {
        font_id: 0,
        glyph_id: 3, // e.g. a space: a valid glyph with no visible outline.
        px: 24,
    };

    let (entry, upload) = cache.get_or_insert(key, || None);
    assert!(entry.draws_nothing());
    assert!(upload.is_none());

    // A second reference is a hit that rasterizes nothing.
    let (entry2, upload2) = cache.get_or_insert(key, || panic!("empty glyph must not re-rasterize"));
    assert!(entry2.draws_nothing());
    assert!(upload2.is_none());
}

#[test]
fn a_dropped_glyph_is_a_cache_hit_that_rasterizes_nothing() {
    // Tiny atlas: a 10x10 glyph (plus gutter) cannot be packed, so it overflows.
    let mut cache = GlyphCache::new(ShelfAllocator::new(8, 8, 1));
    let key = GlyphKey {
        font_id: 0,
        glyph_id: 99,
        px: 48,
    };

    let mut calls = 0;
    let (e1, up1) = cache.get_or_insert(key, || {
        calls += 1;
        Some(raster(10, 10))
    });
    assert_eq!(calls, 1);
    assert!(e1.draws_nothing(), "an overflowed glyph draws nothing");
    assert!(up1.is_none(), "an overflowed glyph is never uploaded");

    // The sentinel makes the second reference a cache hit: no re-rasterization, no upload.
    let (e2, up2) = cache.get_or_insert(key, || {
        calls += 1;
        panic!("a dropped glyph must not be rasterized again");
    });
    assert_eq!(calls, 1);
    assert!(e2.draws_nothing());
    assert!(up2.is_none());
}

#[test]
fn mark_dropped_turns_a_packed_glyph_into_a_cache_hit_sentinel() {
    let mut cache = GlyphCache::new(ShelfAllocator::new(64, 64, 1));
    let key = GlyphKey {
        font_id: 1,
        glyph_id: 5,
        px: 16,
    };

    let (entry, upload) = cache.get_or_insert(key, || Some(raster(4, 4)));
    assert!(!entry.draws_nothing());
    assert!(upload.is_some());

    // Simulate a failed GPU upload: the entry is replaced with a sentinel.
    cache.mark_dropped(key);
    let (entry2, upload2) = cache.get_or_insert(key, || panic!("a dropped glyph must not re-rasterize"));
    assert!(entry2.draws_nothing());
    assert!(upload2.is_none());
}

#[test]
fn glyph_quad_snaps_the_pen_and_applies_bearings() {
    // 'A'-like entry: 20x30 bitmap, left bearing 3, top bearing -28 (above baseline).
    let entry = GlyphEntry {
        cell: AtlasRect {
            x: 5,
            y: 7,
            width: 20,
            height: 30,
        },
        left: 3,
        top: -28,
    };

    let (position, scale) = glyph_quad(10.4, 50.6, &entry);
    // round(10.4) = 10, + left 3 = 13; round(50.6) = 51, + top -28 = 23.
    assert_eq!(position, Vector2::new(13.0, 23.0));
    assert_eq!(scale, Vector2::new(20.0, 30.0));
}

#[test]
fn glyph_cell_uv_insets_half_a_texel_inside_the_cell() {
    let cell = AtlasRect {
        x: 10,
        y: 20,
        width: 4,
        height: 6,
    };
    let atlas = 1024u32;
    let uv = glyph_cell_uv(cell, atlas);
    let a = atlas as f32;

    // Offset is the cell origin shifted in by half a texel; scale spans one texel short of the cell.
    assert_eq!(uv.z, (10.0 + 0.5) / a);
    assert_eq!(uv.w, (20.0 + 0.5) / a);
    assert_eq!(uv.x, (4.0 - 1.0) / a);
    assert_eq!(uv.y, (6.0 - 1.0) / a);

    // The sampled U/V ranges stay strictly inside the cell [x, x+w] / [y, y+h].
    let u0 = uv.z;
    let u1 = uv.z + uv.x;
    assert!(u0 > 10.0 / a && u1 < 14.0 / a);
    let v0 = uv.w;
    let v1 = uv.w + uv.y;
    assert!(v0 > 20.0 / a && v1 < 26.0 / a);
}
