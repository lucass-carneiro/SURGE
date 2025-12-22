use nalgebra;

/// Database blending mode
#[derive(Debug)]
pub enum BlendingMode {
    None,
    Additive,
    Alpha,
}

/// Controls database creation
#[derive(Debug)]
pub struct CreateInfo {
    pub blending_mode: BlendingMode,
    pub max_sprites: usize,
}

/// Controls sprite update data
#[derive(Debug)]
pub struct UpdateInfo {
    pub position: nalgebra::Vector2<f32>,
    pub scale: nalgebra::Vector2<f32>,
    pub z: f32,
    pub texture_id: usize,
    pub color_multiplier: nalgebra::Point4<f32>,
}

/// Creates an orthographic projection for 2D rendering.
/// It places the origin on upper left corner of the game window
/// and normalizes the frustum to be on the 1.0 (near) to 0.0 (far) range
///
/// # Parameters:
/// * `dims`: Screen dimensions.
pub fn make_ortho_projection(width: f32, height: f32) -> nalgebra::Matrix4<f32> {
    nalgebra::Matrix4::new_orthographic(0.0f32, width, 0.0f32, height, 1.0f32, 0.0f32)
}

/// Creates a view matrix for 2D rendering
///
/// # Parameters:
/// `eye`: Position of the camera in 2D coordinates.
pub fn make_view(eye: nalgebra::Point2<f32>) -> nalgebra::Matrix4<f32> {
    let eye_3d = nalgebra::Point3::new(eye[0], eye[1], 1.0f32);
    let target = nalgebra::Point3::new(eye[0], eye[1], 0.0f32);
    let up = nalgebra::Vector3::new(0.0f32, 1.0f32, 0.0f32);
    nalgebra::Matrix4::look_at_rh(&eye_3d, &target, &up)
}

/// Crete a model matrix for a given sprite
pub fn make_model_matrix(
    position: nalgebra::Vector2<f32>,
    scale: nalgebra::Vector2<f32>,
    z: f32,
) -> nalgebra::Matrix4<f32> {
    let mv = nalgebra::Vector3::new(position[0], position[1], z);
    let sc = nalgebra::Vector3::new(scale[0], scale[1], 1.0f32);
    nalgebra::Matrix4::identity()
        .append_translation(&mv)
        .append_nonuniform_scaling(&sc)
}
