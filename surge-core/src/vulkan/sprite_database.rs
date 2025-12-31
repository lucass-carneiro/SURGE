use super::{AllocatedImage, VulkanContext, ctx_buffer::Buffer};
use crate::errors::VulkanError;
use ash::vk::{self, DescriptorType};
use nalgebra;
use std::{cell::RefCell, mem::size_of, sync::Arc};
use vk_mem;

/// Database blending mode
#[derive(Debug)]
pub enum BlendingMode {
    None,
    Additive,
    Alpha,
}

impl BlendingMode {
    pub fn default() -> Self {
        BlendingMode::Alpha
    }
}

/// Controls database creation
#[derive(Debug)]
pub struct CreateInfo {
    pub blending_mode: BlendingMode,
    pub max_sprites: u32,
    pub window_width: f32,
    pub window_height: f32,
}

/// Controls sprite update data
#[derive(Debug)]
pub struct UpdateInfo {
    pub position: nalgebra::Vector2<f32>,
    pub scale: nalgebra::Vector2<f32>,
    pub z: f32,
    pub texture_id: usize,
    pub color_multiplier: nalgebra::Vector4<f32>,
}

impl UpdateInfo {
    pub fn default() -> Self {
        Self {
            position: nalgebra::Vector2::new(0.0f32, 0.0f32),
            scale: nalgebra::Vector2::new(100.0f32, 100.0f32),
            z: 0.0f32,
            texture_id: 0,
            color_multiplier: nalgebra::Vector4::from_element(1.0f32),
        }
    }
}

/// Data shared across all sprite instances in a frame.
/// Provided via UBO.
struct FrameGlobals {
    view: nalgebra::Matrix4<f32>,
    proj: nalgebra::Matrix4<f32>,
}

const FRAME_GLOBALS_STRUCT_SIZE: usize = size_of::<FrameGlobals>();

/// Data that describes each sprite instance
/// Provided via SSBO (needs to be 16 byte aligned)
/// One entry per sprite
/// Indexed by gl_InstanceID (or equivalent)
#[repr(align(16))]
struct InstanceRecord {
    model: nalgebra::Matrix4<f32>,
    color: nalgebra::Vector4<f32>,
    material_id: u32,
}

const INSTANCE_RECORD_STRUCT_SIZE: usize = size_of::<InstanceRecord>();

/// TODO: Uhhhhmmmmm ?
struct TextureRecord {
    staging_buffer: Buffer,
    image: AllocatedImage,
}

pub struct SpriteDatabase {
    /// The vulkan context that created this database
    context: Arc<RefCell<VulkanContext>>,

    /// Creation info
    ci: CreateInfo,

    /// The number of elements currently in the databasae
    occupancy: u32,

    /// UBO storing frame global data (see FrameGlobals)
    frame_globals_ubo: Buffer,

    /// SSBO storing instance data (see InstanceRecord)
    instance_records_ssbo: Buffer,

    /// Address of SSBO storing instance data
    instance_records_ssbo_address: vk::DeviceAddress,

    /// Database descriptor pool
    desc_pool: vk::DescriptorPool,

    /// Descriptor set layout for the UBO
    ubo_desc_set_layout: vk::DescriptorSetLayout,

    /// Descriptor set layout for the sampler and textures
    img_desc_set_layout: vk::DescriptorSetLayout,

    /// UBO descriptor set
    ubo_desc_set: vk::DescriptorSet,

    /// Sampler and textures descriptor set
    img_desc_set: vk::DescriptorSet,
}

impl SpriteDatabase {
    pub fn new(context: Arc<RefCell<VulkanContext>>, ci: CreateInfo) -> Result<Self, VulkanError> {
        log::info!("Creating sprite database");

        //Frame globals UBO
        let frame_globals_ubo = {
            let bci = vk::BufferCreateInfo::default()
                .size(FRAME_GLOBALS_STRUCT_SIZE as u64)
                .usage(vk::BufferUsageFlags::UNIFORM_BUFFER);

            let bai = vk_mem::AllocationCreateInfo {
                usage: vk_mem::MemoryUsage::Auto,
                ..Default::default()
            };

            context.borrow().create_buffer(&bci, &bai)
        }?;

        //Instance record SSBO
        let instance_records_ssbo = {
            let bci = vk::BufferCreateInfo::default()
                .size((INSTANCE_RECORD_STRUCT_SIZE * (ci.max_sprites as usize)) as u64)
                .usage(
                    vk::BufferUsageFlags::STORAGE_BUFFER
                        | vk::BufferUsageFlags::SHADER_DEVICE_ADDRESS,
                );

            let bai = vk_mem::AllocationCreateInfo {
                flags: vk_mem::AllocationCreateFlags::MAPPED
                    | vk_mem::AllocationCreateFlags::HOST_ACCESS_SEQUENTIAL_WRITE,
                usage: vk_mem::MemoryUsage::Auto,
                ..Default::default()
            };

            context.borrow().create_buffer(&bci, &bai)
        }?;

        let instance_records_ssbo_address = {
            let ai = vk::BufferDeviceAddressInfo::default().buffer(instance_records_ssbo.buffer);
            unsafe { context.borrow().device.get_buffer_device_address(&ai) }
        };

        // Descriptor pool
        let desc_pool = {
            let sizes = [
                vk::DescriptorPoolSize::default()
                    .ty(DescriptorType::UNIFORM_BUFFER)
                    .descriptor_count(1),
                vk::DescriptorPoolSize::default()
                    .ty(DescriptorType::SAMPLER)
                    .descriptor_count(1),
                vk::DescriptorPoolSize::default()
                    .ty(DescriptorType::SAMPLED_IMAGE)
                    .descriptor_count(ci.max_sprites),
            ];

            let dpci = vk::DescriptorPoolCreateInfo::default()
                .max_sets(2)
                .pool_sizes(&sizes);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_descriptor_pool(&dpci, None)
                    .map_err(|e| VulkanError::DescriptorPoolCreationError(e))
            }
        }?;

        // Desc. set layouts
        let ubo_desc_set_layout = {
            let bindings = [vk::DescriptorSetLayoutBinding {
                binding: 0,
                descriptor_type: vk::DescriptorType::UNIFORM_BUFFER,
                descriptor_count: 1,
                stage_flags: vk::ShaderStageFlags::VERTEX,
                ..Default::default()
            }];

            let dslci = vk::DescriptorSetLayoutCreateInfo::default().bindings(&bindings);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_descriptor_set_layout(&dslci, None)
                    .map_err(|e| VulkanError::DescriptorSetLayoutCreationError(e))
            }
        }?;

        let img_desc_set_layout = {
            let bindings = [
                vk::DescriptorSetLayoutBinding {
                    binding: 0,
                    descriptor_type: vk::DescriptorType::SAMPLER,
                    descriptor_count: 1,
                    stage_flags: vk::ShaderStageFlags::FRAGMENT,
                    ..Default::default()
                },
                vk::DescriptorSetLayoutBinding {
                    binding: 1,
                    descriptor_type: vk::DescriptorType::SAMPLED_IMAGE,
                    descriptor_count: ci.max_sprites,
                    stage_flags: vk::ShaderStageFlags::FRAGMENT,
                    ..Default::default()
                },
            ];

            let dsbf = [
                vk::DescriptorBindingFlags::empty(),
                vk::DescriptorBindingFlags::PARTIALLY_BOUND
                    | vk::DescriptorBindingFlags::VARIABLE_DESCRIPTOR_COUNT,
            ];

            let mut dbfci =
                vk::DescriptorSetLayoutBindingFlagsCreateInfo::default().binding_flags(&dsbf);

            let dslci = vk::DescriptorSetLayoutCreateInfo::default()
                .bindings(&bindings)
                .push_next(&mut dbfci);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_descriptor_set_layout(&dslci, None)
                    .map_err(|e| VulkanError::DescriptorSetLayoutCreationError(e))
            }
        }?;

        // Desc. sets
        let ubo_desc_set = {
            let layouts = [ubo_desc_set_layout];

            let ai = vk::DescriptorSetAllocateInfo::default()
                .descriptor_pool(desc_pool)
                .set_layouts(&layouts);

            unsafe {
                context
                    .borrow()
                    .device
                    .allocate_descriptor_sets(&ai)
                    .map_err(|e| VulkanError::DescriptorSetAllocationError(e))
            }
        }?[0];

        let img_desc_set = {
            let counts = [ci.max_sprites];

            let mut dsvdcai = vk::DescriptorSetVariableDescriptorCountAllocateInfo::default()
                .descriptor_counts(&counts);

            let layouts = [img_desc_set_layout];

            let ai = vk::DescriptorSetAllocateInfo::default()
                .descriptor_pool(desc_pool)
                .set_layouts(&layouts)
                .push_next(&mut dsvdcai);

            unsafe {
                context
                    .borrow()
                    .device
                    .allocate_descriptor_sets(&ai)
                    .map_err(|e| VulkanError::DescriptorSetAllocationError(e))
            }
        }?[0];

        Ok(Self {
            context,
            ci,
            occupancy: 0,
            frame_globals_ubo,
            instance_records_ssbo,
            instance_records_ssbo_address,
            desc_pool,
            ubo_desc_set_layout,
            img_desc_set_layout,
            ubo_desc_set,
            img_desc_set,
        })
    }
}

impl Drop for SpriteDatabase {
    fn drop(&mut self) {
        log::debug!("Destroying sprite database");

        unsafe {
            self.context
                .borrow()
                .device
                .destroy_descriptor_set_layout(self.img_desc_set_layout, None);

            self.context
                .borrow()
                .device
                .destroy_descriptor_set_layout(self.ubo_desc_set_layout, None);

            self.context
                .borrow()
                .device
                .destroy_descriptor_pool(self.desc_pool, None);
        }
        self.context
            .borrow()
            .destroy_buffer(&mut self.instance_records_ssbo);
        self.context
            .borrow()
            .destroy_buffer(&mut self.frame_globals_ubo);
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
pub fn make_view(eye: nalgebra::Vector2<f32>) -> nalgebra::Matrix4<f32> {
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
