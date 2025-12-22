use super::{AllocatedImage, VulkanContext, ctx_buffer::Buffer};
use ash::vk;
use nalgebra;
use std::mem::size_of;

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

/// Per sprite shader data
#[repr(align(16))]
struct SpriteData {
    model_matrix: nalgebra::Matrix4<f32>,
    color_multiplier: nalgebra::Vector4<f32>,
}

const SPRITE_DATA_SIZE: usize = size_of::<SpriteData>();

/// Per database shader data. TODO: This should be an UBO
struct PushConstants {
    projection: nalgebra::Matrix4<f32>,
    view: nalgebra::Matrix4<f32>,
    sprite_data_buffer_address: vk::DeviceAddress,
}

const PUSH_CONSTANTS_SIZE: usize = size_of::<PushConstants>();

struct RawImageData {
    image_data: Buffer,
    image_extent: vk::Extent3D,
    size: usize,
}

struct SpriteDatabase {
    /// How many sprites we can add
    max_sprites: usize,

    /// CPU staging buffer for sprite data
    cpu_data_buffer: Buffer,

    /// Num of sprite data elms. curr. stored in cpu_data_buffer
    cpu_data_buffer_size: usize,

    /// CPU buffer with sprite data
    gpu_data_buffer: Buffer,

    /// Num of sprite data elms. sent to gpu_data_buffer
    gpu_data_buffer_size: usize,

    /// Address of GPU buffer with sprite data.
    gpu_data_buffer_address: vk::DeviceAddress,

    /// CPU staging image data
    img_src_buffers: Vec<RawImageData>,

    ///GPU image data
    img_dst_images: Vec<AllocatedImage>,

    /// Database command buffer
    cmd_buff: vk::CommandBuffer,

    /// Database command pool
    cmd_pool: vk::CommandPool,

    /// Image transfer fence
    image_transfer_fence: vk::Fence,

    /// Data transfer fence
    data_transfer_fence: vk::Fence,

    /// Image descriptor infos
    img_desc_infos: Vec<vk::DescriptorImageInfo>,

    /// Image sprite samplers
    img_samplers: Vec<vk::Sampler>,

    /// Image descriptor layout
    img_desc_layout: vk::DescriptorSetLayout,

    /// Image descriptor set
    img_desc_set: vk::DescriptorSet,

    /// Database pipeline layout
    pipeline_layout: vk::PipelineLayout,

    /// Database pipeline
    pipeline: vk::Pipeline,
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
