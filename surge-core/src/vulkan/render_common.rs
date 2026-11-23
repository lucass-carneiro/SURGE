//! Pure, GPU-free helpers shared by every instanced-quad database (`sprite_database`,
//! `font_database`): the orthographic projection and view, the per-instance model matrix, the
//! letterbox mapping from design resolution to the swapchain, and the sub-rect UV transform. They
//! live here so both databases call one copy rather than duplicating the math.

use ash::vk;

/// Creates an orthographic projection for 2D rendering.
/// It places the origin on upper left corner of the game window
/// with +x pointing right, +y pointing down and +z pointing into
/// the screen. Composed with `make_view`'s camera (eye at z = 1,
/// looking at z = 0), a `z` passed to `add_instance` in `[0, 1]`
/// maps to Vulkan NDC/depth `[0, 1]`, with `z = 0` frontmost
/// (depth = 1) and `z = 1` backmost (depth = 0).
///
/// `nalgebra::Matrix4::new_orthographic` emits the GL convention
/// (NDC z in `[-1, 1]`), which Vulkan does not clip-space-remap on
/// its own — Vulkan's NDC/depth range is `[0, 1]` directly. Using
/// the GL matrix as-is silently clips any z whose GL NDC z is
/// negative. This builds the z row for Vulkan's convention directly
/// instead.
///
/// # Parameters:
/// * `width`, `height`: Screen dimensions.
pub fn make_ortho_projection(width: f32, height: f32) -> nalgebra::Matrix4<f32> {
    #[rustfmt::skip]
    let proj = nalgebra::Matrix4::new(
        2.0f32 / width, 0.0,             0.0, -1.0,
        0.0,            2.0f32 / height, 0.0, -1.0,
        0.0,            0.0,            -1.0,  0.0,
        0.0,            0.0,             0.0,  1.0,
    );
    proj
}

/// Creates a view matrix for 2D rendering
///
/// # Parameters:
/// `eye`: Position of the camera in 2D coordinates.
pub fn make_view(eye: nalgebra::Vector2<f32>) -> nalgebra::Matrix4<f32> {
    let eye_3d = nalgebra::Point3::new(eye[0], eye[1], 1.0f32);
    let target = nalgebra::Point3::new(eye[0], eye[1], 0.0f32);
    let up = nalgebra::Vector3::new(0.0f32, 1.0f32, 0.0f32);
    nalgebra::Matrix4::look_at_rh(&eye_3d, &target, &up)
}

/// Create a model matrix for a given quad (a sprite, or a glyph bitmap).
pub fn make_model_matrix(
    position: nalgebra::Vector2<f32>,
    scale: nalgebra::Vector2<f32>,
    z: f32,
) -> nalgebra::Matrix4<f32> {
    let mv = nalgebra::Vector3::new(position[0], position[1], z);
    let sc = nalgebra::Vector3::new(scale[0], scale[1], 1.0f32);
    nalgebra::Matrix4::identity()
        .append_nonuniform_scaling(&sc)
        .append_translation(&mv)
}

/// Converts a sub-rect of a `texture`-sized image (all in texels) into the UV transform
/// `(scale.xy, offset.xy)` the vertex shader applies. Derivation in `extra/subimage.wls`.
/// Returns `None` if the sub-rect is not fully inside the texture; a sub-rect that
/// ends exactly on the texture edge is inside.
pub(crate) fn subtexture_uv_transform(
    origin: [u32; 2],
    extent: [u32; 2],
    texture: [u32; 2],
) -> Option<nalgebra::Vector4<f32>> {
    let fits = |o: u32, e: u32, t: u32| o.checked_add(e).is_some_and(|end| end <= t);

    if !fits(origin[0], extent[0], texture[0]) || !fits(origin[1], extent[1], texture[1]) {
        return None;
    }

    Some(nalgebra::Vector4::new(
        extent[0] as f32 / texture[0] as f32,
        extent[1] as f32 / texture[1] as f32,
        origin[0] as f32 / texture[0] as f32,
        origin[1] as f32 / texture[1] as f32,
    ))
}

/// Largest rectangle with the design resolution's aspect ratio that fits in `target`, centered.
/// Returns `(x, y, width, height)` in target pixels; the leftover bars keep the clear color.
pub(crate) fn letterbox(design_w: f32, design_h: f32, target: vk::Extent2D) -> (i32, i32, u32, u32) {
    let scale = (target.width as f32 / design_w).min(target.height as f32 / design_h);
    let width = ((design_w * scale).round() as u32).clamp(1, target.width.max(1));
    let height = ((design_h * scale).round() as u32).clamp(1, target.height.max(1));
    let x = ((target.width - width.min(target.width)) / 2) as i32;
    let y = ((target.height - height.min(target.height)) / 2) as i32;
    (x, y, width, height)
}

#[cfg(test)]
#[path = "render_common_tests.rs"]
mod tests;
