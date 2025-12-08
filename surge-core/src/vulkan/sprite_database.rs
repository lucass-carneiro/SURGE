use nalgebra::{Vector2 as Vec2, Vector4 as Vec4};

/// Database blending mode
#[derive(Debug)]
pub enum BlendingMode {
    None,
    Additive,
    Alpha,
}

/// Controls database creation
#[derive(Debug)]
pub struct SpriteDatabaseCreateInfo {
    pub blending_mode: BlendingMode,
    pub max_sprites: usize,
}

/// Controls sprite update data
#[derive(Debug)]
pub struct SpriteDatabaseUpdateInfo {
    pub position: Vec2<f32>,
    pub scale: Vec2<f32>,
    pub z: f32,
    pub texture_id: usize,
    pub color_multiplier: Vec4<f32>,
}
