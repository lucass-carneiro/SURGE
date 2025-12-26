use super::{AllocatedImage, VulkanContext, ctx_buffer::Buffer};
use crate::errors::VulkanError;
use ash::vk::{self, CommandPoolCreateFlags};
use nalgebra;
use std::{mem::size_of, sync::Arc};
use vk_mem::{Alloc, AllocationCreateFlags};

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
    /// The vulkan context that created this database
    context: Arc<VulkanContext>,

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

    /// Database command pool
    cmd_pool: vk::CommandPool,

    /// Database command buffer
    cmd_buff: vk::CommandBuffer,

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

impl SpriteDatabase {
    pub fn new(context: Arc<VulkanContext>, ci: CreateInfo) -> Result<Self, VulkanError> {
        let buffer_size = (ci.max_sprites * SPRITE_DATA_SIZE) as u64;

        // CPU data buffer
        let cpu_data_buffer = {
            let buffer_info = vk::BufferCreateInfo {
                size: buffer_size,
                usage: vk::BufferUsageFlags::TRANSFER_SRC,
                ..Default::default()
            };

            let create_info = vk_mem::AllocationCreateInfo {
                usage: vk_mem::MemoryUsage::Auto,
                flags: AllocationCreateFlags::HOST_ACCESS_SEQUENTIAL_WRITE
                    | AllocationCreateFlags::MAPPED,
                ..Default::default()
            };

            let (buffer, allocation) = unsafe {
                context
                    .memory_allocator
                    .create_buffer(&buffer_info, &create_info)
                    .map_err(|e| VulkanError::BufferAllocationError(e))
            }?;

            Buffer { buffer, allocation }
        };

        // GPU data buffer
        let gpu_data_buffer = context.create_buffer(
            buffer_size,
            vk::BufferUsageFlags::STORAGE_BUFFER
                | vk::BufferUsageFlags::TRANSFER_DST
                | vk::BufferUsageFlags::SHADER_DEVICE_ADDRESS,
            vk_mem::MemoryUsage::AutoPreferDevice,
        )?;

        let gpu_data_buffer_address = {
            let ai = vk::BufferDeviceAddressInfo {
                buffer: gpu_data_buffer.buffer,
                ..Default::default()
            };

            unsafe { context.device.get_buffer_device_address(&ai) }
        };

        // Command pool
        let cmd_pool = {
            let ci = vk::CommandPoolCreateInfo {
                queue_family_index: context.indices.transfer,
                flags: CommandPoolCreateFlags::RESET_COMMAND_BUFFER,
                ..Default::default()
            };
            unsafe {
                context
                    .device
                    .create_command_pool(&ci, None)
                    .map_err(|e| VulkanError::CommandPoolCreation(e))
            }
        }?;

        let cmd_buff = {
            let ai = vk::CommandBufferAllocateInfo {
                command_pool: cmd_pool,
                level: vk::CommandBufferLevel::PRIMARY,
                command_buffer_count: 1,
                ..Default::default()
            };

            unsafe {
                context
                    .device
                    .allocate_command_buffers(&ai)
                    .map_err(|e| VulkanError::CommandBufferAllocation(e))
            }
        }?[0];

        // Image transfer fence
        let image_transfer_fence = {
            let ci = vk::FenceCreateInfo {
                flags: vk::FenceCreateFlags::SIGNALED,
                ..Default::default()
            };

            unsafe {
                context
                    .device
                    .create_fence(&ci, None)
                    .map_err(|e| VulkanError::FenceCreationError(e))
            }
        }?;

        // Image transfer fence
        let data_transfer_fence = {
            let ci = vk::FenceCreateInfo {
                flags: vk::FenceCreateFlags::SIGNALED,
                ..Default::default()
            };

            unsafe {
                context
                    .device
                    .create_fence(&ci, None)
                    .map_err(|e| VulkanError::FenceCreationError(e))
            }
        }?;

        // Texture samplers
        // TODO: choose filters. do mip-maps
        let img_samplers = {
            let sci = vk::SamplerCreateInfo {
                mag_filter: vk::Filter::NEAREST,
                min_filter: vk::Filter::NEAREST,
                ..Default::default()
            };

            let mut samplers = Vec::new();

            for i in 0..ci.max_sprites {
                let sampler = unsafe {
                    context
                        .device
                        .create_sampler(&sci, None)
                        .map_err(|e| VulkanError::SamplerCreationError(e))
                }?;

                samplers.push(sampler);
            }

            samplers
        };

        // Descriptor set pool allocator

        // Image Descriptor set layout
        let img_desc_layout = {
            let texture_descriptor_binding = [vk::DescriptorSetLayoutBinding {
                binding: 0,
                descriptor_type: vk::DescriptorType::COMBINED_IMAGE_SAMPLER,
                descriptor_count: ci.max_sprites as u32,
                stage_flags: vk::ShaderStageFlags::FRAGMENT,
                p_immutable_samplers: img_samplers.as_ptr(),
                ..Default::default()
            }];

            let texture_binding_flags = [vk::DescriptorBindingFlags::VARIABLE_DESCRIPTOR_COUNT
                | vk::DescriptorBindingFlags::PARTIALLY_BOUND];

            let mut texture_descriptor_set_layout_binding_flags =
                vk::DescriptorSetLayoutBindingFlagsCreateInfo {
                    binding_count: texture_binding_flags.len() as u32,
                    p_binding_flags: texture_binding_flags.as_ptr(),
                    ..Default::default()
                };

            let texture_descriptor_set_layout_info = vk::DescriptorSetLayoutCreateInfo::default()
                .bindings(&texture_descriptor_binding)
                .push_next(&mut texture_descriptor_set_layout_binding_flags);

            unsafe {
                context
                    .device
                    .create_descriptor_set_layout(&texture_descriptor_set_layout_info, None)
                    .map_err(|e| VulkanError::DescriptorSetLayoutCreationError(e))
            }
        }?;

        // Descriptor set
        {
            let descriptor_counts = [ci.max_sprites as u32];

            let letvariable_descriptor_count_info =
                vk::DescriptorSetVariableDescriptorCountAllocateInfo {
                    descriptor_set_count: descriptor_counts.len() as u32,
                    p_descriptor_counts: descriptor_counts.as_ptr(),
                    ..Default::default()
                };

            // Allocate descriptor
        }

        Ok(Self {
            context,
            max_sprites: ci.max_sprites,
            cpu_data_buffer,
            cpu_data_buffer_size: 0,
            gpu_data_buffer,
            gpu_data_buffer_size: 0,
            gpu_data_buffer_address,
            img_src_buffers: Vec::with_capacity(ci.max_sprites),
            img_dst_images: Vec::with_capacity(ci.max_sprites),
            cmd_pool,
            cmd_buff,
            image_transfer_fence,
            data_transfer_fence,
            img_desc_infos: Vec::with_capacity(ci.max_sprites),
            img_samplers,
            img_desc_layout,
            img_desc_set: Default::default(),
            pipeline_layout: Default::default(),
            pipeline: Default::default(),
        })
    }
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
