use nalgebra;
use serde::{Deserialize, Serialize};

/// Database blending mode
#[derive(Debug, Clone, Copy, Default, Serialize, Deserialize)]
pub enum BlendingMode {
    None,
    Additive,
    #[default]
    Alpha,
}

/// Texture filtering modes
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum TextureFilteringMode {
    Nearest,
    Linear,
}

/// Texture anisotropic  filtering level
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum TextureFilteringLevel {
    X0 = 0,
    X1 = 1,
    X2 = 2,
    X4 = 4,
    X8 = 8,
    X16 = 16,
}

/// Controls database creation
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct CreateInfo {
    pub blending_mode: BlendingMode,
    pub texture_filtering_mode: TextureFilteringMode,
    pub texture_filtering_level: TextureFilteringLevel,
    pub max_sprites: u32,
    pub max_textures: u32,
    pub window_width: f32,
    pub window_height: f32,
}

#[derive(Debug, Clone, Copy, Default, Serialize, Deserialize)]
pub struct SubTextureInfo {
    pub origin: nalgebra::Vector2<u32>,
    pub extent: nalgebra::Vector2<u32>,
}

impl SubTextureInfo {
    pub fn origin(mut self, origin: nalgebra::Vector2<u32>) -> Self {
        self.origin = origin;
        self
    }

    pub fn extent(mut self, extent: nalgebra::Vector2<u32>) -> Self {
        self.extent = extent;
        self
    }
}

/// Specifies the parameters of an instance to be added to the database
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct InstanceInfo {
    pub position: nalgebra::Vector2<f32>,
    pub scale: nalgebra::Vector2<f32>,
    pub z: f32,
    pub texture_id: usize,
    pub color_multiplier: nalgebra::Vector4<f32>,
    pub subtexture_info: Option<SubTextureInfo>,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn instance_info_round_trips_through_postcard() {
        let original = InstanceInfo {
            position: nalgebra::Vector2::new(12.5, -3.0),
            scale: nalgebra::Vector2::new(64.0, 32.0),
            z: 0.25,
            texture_id: 7,
            color_multiplier: nalgebra::Vector4::new(1.0, 0.5, 0.25, 1.0),
            subtexture_info: Some(
                SubTextureInfo::default()
                    .origin(nalgebra::Vector2::new(16, 32))
                    .extent(nalgebra::Vector2::new(8, 8)),
            ),
        };

        let bytes = postcard::to_stdvec(&original).unwrap();
        let decoded: InstanceInfo = postcard::from_bytes(&bytes).unwrap();

        assert_eq!(decoded.position, original.position);
        assert_eq!(decoded.scale, original.scale);
        assert_eq!(decoded.z, original.z);
        assert_eq!(decoded.texture_id, original.texture_id);
        assert_eq!(decoded.color_multiplier, original.color_multiplier);
        let (sub, orig_sub) = (
            decoded.subtexture_info.unwrap(),
            original.subtexture_info.unwrap(),
        );
        assert_eq!(sub.origin, orig_sub.origin);
        assert_eq!(sub.extent, orig_sub.extent);
    }
}
