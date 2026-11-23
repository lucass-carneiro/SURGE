use super::*;

fn ext(width: u32, height: u32) -> vk::Extent2D {
    vk::Extent2D { width, height }
}

#[test]
fn subtexture_uv_transform_maps_rect_to_normalised_scale_and_offset() {
    let uv = subtexture_uv_transform([100, 50], [200, 25], [400, 100]).unwrap();
    assert_eq!(uv, nalgebra::Vector4::new(0.5, 0.25, 0.25, 0.5));
}

#[test]
fn subtexture_uv_transform_accepts_rect_ending_exactly_on_the_edge() {
    // The shipped 12502x502 sheet's last frame ends at 12500, but a rect may end at 12502.
    assert!(subtexture_uv_transform([12004, 2], [498, 500], [12502, 502]).is_some());
    assert!(subtexture_uv_transform([0, 0], [400, 100], [400, 100]).is_some());
}

#[test]
fn subtexture_uv_transform_rejects_out_of_bounds_and_overflowing_rects() {
    assert!(subtexture_uv_transform([1, 0], [400, 100], [400, 100]).is_none());
    assert!(subtexture_uv_transform([0, 1], [400, 100], [400, 100]).is_none());
    assert!(subtexture_uv_transform([400, 0], [1, 1], [400, 100]).is_none());
    // `origin + extent` overflows u32; must be rejected, not panic in debug builds.
    assert!(subtexture_uv_transform([u32::MAX, 0], [2, 1], [400, 100]).is_none());
}

#[test]
fn letterbox_identity_when_extent_matches_design() {
    assert_eq!(letterbox(500.0, 800.0, ext(500, 800)), (0, 0, 500, 800));
}

#[test]
fn letterbox_pillarboxes_portrait_design_on_landscape_monitor() {
    // 500x800 into 1920x1080: height-bound, scale 1.35 -> 675x1080, centered horizontally.
    assert_eq!(letterbox(500.0, 800.0, ext(1920, 1080)), (622, 0, 675, 1080));
}

#[test]
fn letterbox_stays_inside_target() {
    for (w, h) in [(1, 1), (7, 3), (3840, 2160), (800, 500)] {
        let (x, y, lw, lh) = letterbox(500.0, 800.0, ext(w, h));
        assert!(x >= 0 && y >= 0);
        assert!(x as u32 + lw <= w && y as u32 + lh <= h, "{w}x{h}");
    }
}

/// Runs a quad's z through the same proj * view * model chain the engine builds at draw time and
/// returns the resulting Vulkan NDC/depth z (post perspective-divide).
fn ndc_z(z: f32, width: f32, height: f32) -> f32 {
    let model = make_model_matrix(
        nalgebra::Vector2::new(0.0f32, 0.0f32),
        nalgebra::Vector2::new(1.0f32, 1.0f32),
        z,
    );
    let view = make_view(nalgebra::Vector2::from_element(0.0f32));
    let proj = make_ortho_projection(width, height);

    let clip = proj * view * model * nalgebra::Vector4::new(0.0f32, 0.0f32, 0.0f32, 1.0f32);

    // Orthographic: w must stay 1, i.e. no perspective divide needed.
    assert!(
        (clip.w - 1.0f32).abs() < 1e-6,
        "orthographic projection introduced a perspective divide: w = {}",
        clip.w
    );

    clip.z
}

#[test]
fn full_z_range_lands_inside_vulkan_depth_bounds() {
    let (width, height) = (500.0f32, 800.0f32);

    // Documented range (CLAUDE.md, make_ortho_projection): z in [0, 1]
    // must map onto Vulkan's valid NDC/depth range [0, 1] in full,
    // not just the [0, 0.5] half the pre-fix GL-convention matrix
    // produced (values above landed in negative NDC and were clipped).
    for &(z, expected_depth) in &[
        (0.00f32, 1.00f32),
        (0.25f32, 0.75f32),
        (0.50f32, 0.50f32),
        (0.75f32, 0.25f32),
        (1.00f32, 0.00f32),
    ] {
        let depth = ndc_z(z, width, height);

        assert!(
            (0.0..=1.0).contains(&depth),
            "z = {z} produced depth {depth}, outside Vulkan's valid [0, 1] range \
             (would be clipped and never rendered)"
        );
        assert!(
            (depth - expected_depth).abs() < 1e-5,
            "z = {z}: expected depth {expected_depth}, got {depth}"
        );
    }
}

#[test]
fn smaller_world_z_draws_in_front() {
    // GREATER_OR_EQUAL depth test + clear depth 0.0: a larger stored
    // depth wins, so smaller world z (foreground, e.g. 2048 pieces at
    // z = 0.0) must map to a larger depth than larger world z
    // (background, e.g. the board at z = 0.5).
    let (width, height) = (500.0f32, 800.0f32);

    let foreground_depth = ndc_z(0.0, width, height);
    let background_depth = ndc_z(0.5, width, height);
    let backmost_depth = ndc_z(1.0, width, height);

    assert!(foreground_depth > background_depth);
    assert!(background_depth > backmost_depth);
}
