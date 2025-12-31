use super::{
    AllocatedImage, DPETH_FORMAT, VulkanContext, ctx_buffer::Buffer,
    ctx_graphics_pipeline::GraphicsPipelineBuilder,
};
use crate::errors::VulkanError;
use ash::vk::{self, DescriptorType};
use nalgebra;
use std::{cell::RefCell, mem::size_of, slice, sync::Arc};
use vk_mem::{self, Alloc};

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
#[repr(align(16))]
struct FrameGlobals {
    view: nalgebra::Matrix4<f32>,
    proj: nalgebra::Matrix4<f32>,
}

const FRAME_GLOBALS_STRUCT_SIZE: u64 = size_of::<FrameGlobals>() as u64;

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

const INSTANCE_RECORD_STRUCT_SIZE: u64 = size_of::<InstanceRecord>() as u64;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
struct PushConstants {
    instance_records_ssbo_address: vk::DeviceAddress,
}

const PUSH_CONSTANTS_STRUCT_SIZE: u32 = size_of::<PushConstants>() as u32;

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

    /// Texture sampler
    texture_sampler: vk::Sampler,

    /// Database descriptor pool
    desc_pool: vk::DescriptorPool,

    /// Descriptor set layout for the UBO
    ubo_desc_set_layout: vk::DescriptorSetLayout,

    /// Descriptor set layout for the sampler and textures
    mat_desc_set_layout: vk::DescriptorSetLayout,

    /// UBO descriptor set
    ubo_desc_set: vk::DescriptorSet,

    /// Sampler and textures descriptor set
    mat_desc_set: vk::DescriptorSet,

    /// Graphics pipeline layout
    pipeline_layout: vk::PipelineLayout,

    /// Graphics pipeline
    pipeline: vk::Pipeline,

    /// Push constant data
    push_constant_data: PushConstants,

    /// Default iamage to use when textures are missing
    default_image: AllocatedImage,
}

impl SpriteDatabase {
    pub fn new(context: Arc<RefCell<VulkanContext>>, ci: CreateInfo) -> Result<Self, VulkanError> {
        log::info!("Creating sprite database");

        //Frame globals UBO
        let frame_globals_ubo = {
            let bci = vk::BufferCreateInfo::default()
                .size(FRAME_GLOBALS_STRUCT_SIZE)
                .usage(vk::BufferUsageFlags::UNIFORM_BUFFER);

            let bai = vk_mem::AllocationCreateInfo {
                flags: vk_mem::AllocationCreateFlags::MAPPED
                    | vk_mem::AllocationCreateFlags::HOST_ACCESS_SEQUENTIAL_WRITE,
                usage: vk_mem::MemoryUsage::Auto,
                ..Default::default()
            };

            context.borrow().create_buffer(&bci, &bai)
        }?;

        //Instance record SSBO
        let instance_records_ssbo = {
            let bci = vk::BufferCreateInfo::default()
                .size(INSTANCE_RECORD_STRUCT_SIZE * (ci.max_sprites as u64))
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

        // Texture sampler
        // TODO: Texture filtering and mip maps
        let texture_sampler = {
            let sci = vk::SamplerCreateInfo::default()
                .mag_filter(vk::Filter::NEAREST)
                .min_filter(vk::Filter::NEAREST);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_sampler(&sci, None)
                    .map_err(|e| VulkanError::SamplerCreationError(e))
            }
        }?;

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

        let mat_desc_set_layout = {
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

        let mat_desc_set = {
            let counts = [ci.max_sprites];

            let mut dsvdcai = vk::DescriptorSetVariableDescriptorCountAllocateInfo::default()
                .descriptor_counts(&counts);

            let layouts = [mat_desc_set_layout];

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

        // Global UBO descriptor update
        {
            let bi = [vk::DescriptorBufferInfo::default()
                .buffer(frame_globals_ubo.buffer)
                .offset(0)
                .range(FRAME_GLOBALS_STRUCT_SIZE)];

            let dsw = [vk::WriteDescriptorSet::default()
                .dst_set(ubo_desc_set)
                .dst_binding(0)
                .descriptor_type(vk::DescriptorType::UNIFORM_BUFFER)
                .descriptor_count(1)
                .buffer_info(&bi)];

            // Write data to buffer
            let view = make_view(nalgebra::Vector2::from_element(0.0f32));
            let proj = make_ortho_projection(ci.window_width, ci.window_height);

            unsafe {
                *(frame_globals_ubo.allocation_info.mapped_data as *mut FrameGlobals) =
                    FrameGlobals { view, proj };
            }

            unsafe { context.borrow().device.update_descriptor_sets(&dsw, &[]) };
        }

        // Sampler descriptor update
        {
            let si = [vk::DescriptorImageInfo::default().sampler(texture_sampler)];

            let dsw = [vk::WriteDescriptorSet::default()
                .dst_set(mat_desc_set)
                .dst_binding(0)
                .descriptor_type(vk::DescriptorType::SAMPLER)
                .descriptor_count(1)
                .image_info(&si)];

            unsafe { context.borrow().device.update_descriptor_sets(&dsw, &[]) };
        }

        // Pipeline layout
        let pipeline_layout = {
            let pcr = [vk::PushConstantRange::default()
                .offset(0)
                .size(PUSH_CONSTANTS_STRUCT_SIZE)
                .stage_flags(vk::ShaderStageFlags::VERTEX)];

            let set_layouts = [ubo_desc_set_layout, mat_desc_set_layout];

            let pci = vk::PipelineLayoutCreateInfo::default()
                .push_constant_ranges(&pcr)
                .set_layouts(&set_layouts);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_pipeline_layout(&pci, None)
                    .map_err(|e| VulkanError::GraphicsPipelineCreationError(e))
            }
        }?;

        // Pipeline
        let pipeline = {
            let vert_shader = context
                .borrow()
                .load_shader_module("shaders/sprite.vert.spv")?;

            let frag_shader = context
                .borrow()
                .load_shader_module("shaders/sprite.frag.spv")?;

            let pb = GraphicsPipelineBuilder::default()
                .set_layout(pipeline_layout)
                .set_shaders(vert_shader, frag_shader)
                .set_input_topology(vk::PrimitiveTopology::TRIANGLE_LIST)
                .set_polygon_mode(vk::PolygonMode::FILL)
                .set_cull_mode(vk::CullModeFlags::NONE, vk::FrontFace::CLOCKWISE)
                .set_multisampling_none()
                .set_blending_alpha() // TODO: take this from creation info
                .set_depth_test_enabled(true, vk::CompareOp::GREATER_OR_EQUAL)
                .set_color_attachment_format(context.borrow().swapchain_data.format)
                .set_depth_format(DPETH_FORMAT);

            let pipeline = context.borrow().create_graphics_pipeline(pb)?;

            context.borrow().unload_shader_module(frag_shader);
            context.borrow().unload_shader_module(vert_shader);

            pipeline
        };

        let default_image = {
            let extent = vk::Extent3D {
                width: 100,
                height: 100,
                depth: 1,
            };

            let image_ci = vk::ImageCreateInfo {
                image_type: vk::ImageType::TYPE_2D,
                format: vk::Format::R8G8B8A8_UNORM,
                extent: extent,
                mip_levels: 1,
                array_layers: 1,
                samples: vk::SampleCountFlags::TYPE_1,
                tiling: vk::ImageTiling::OPTIMAL,
                usage: vk::ImageUsageFlags::SAMPLED,
                ..Default::default()
            };

            let alloc_ci = vk_mem::AllocationCreateInfo {
                usage: vk_mem::MemoryUsage::AutoPreferDevice,
                ..Default::default()
            };

            let (image, image_memory) = unsafe {
                context
                    .borrow()
                    .memory_allocator
                    .create_image(&image_ci, &alloc_ci)
                    .map_err(|e| VulkanError::DepthImageCreationError(e))
            }?;

            let sr = vk::ImageSubresourceRange {
                aspect_mask: vk::ImageAspectFlags::COLOR,
                base_mip_level: 0,
                level_count: 1,
                base_array_layer: 0,
                layer_count: 1,
                ..Default::default()
            };

            let ivci = vk::ImageViewCreateInfo {
                view_type: vk::ImageViewType::TYPE_2D,
                format: vk::Format::R8G8B8A8_UNORM,
                subresource_range: sr,
                image: image,
                ..Default::default()
            };

            let image_view = unsafe {
                context
                    .borrow()
                    .device
                    .create_image_view(&ivci, None)
                    .map_err(|e| VulkanError::SwapchainCreationError(e))
            }?;

            AllocatedImage {
                image,
                image_view,
                image_memory,
            }
        };

        let push_constant_data = PushConstants {
            instance_records_ssbo_address,
        };

        Ok(Self {
            context,
            ci,
            occupancy: 0,
            frame_globals_ubo,
            instance_records_ssbo,
            texture_sampler,
            desc_pool,
            ubo_desc_set_layout,
            mat_desc_set_layout,
            ubo_desc_set,
            mat_desc_set,
            pipeline_layout,
            pipeline,
            push_constant_data,
            default_image,
        })
    }

    /// Temporary test function, adds a sprite with random texture. Will be removed
    pub fn tmp_test(&mut self) {
        // Instance data update
        {
            let instance_records_slice: &mut [InstanceRecord] = unsafe {
                slice::from_raw_parts_mut(
                    self.instance_records_ssbo.allocation_info.mapped_data as *mut InstanceRecord,
                    self.ci.max_sprites as usize,
                )
            };

            let model = make_model_matrix(
                nalgebra::Vector2::from_element(0.0f32),
                nalgebra::Vector2::from_element(100.0f32),
                0.1f32,
            );

            let color = nalgebra::Vector4::from_element(1.0f32);

            let record = InstanceRecord {
                model,
                color,
                material_id: 0,
            };

            instance_records_slice[self.occupancy as usize] = record;
        }

        // Image data update
        {
            let si = [vk::DescriptorImageInfo::default()
                .image_view(self.default_image.image_view)
                .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)];

            let dsw = [vk::WriteDescriptorSet::default()
                .dst_set(self.mat_desc_set)
                .dst_binding(1)
                .descriptor_type(vk::DescriptorType::SAMPLED_IMAGE)
                .descriptor_count(1)
                .image_info(&si)];

            unsafe {
                self.context
                    .borrow()
                    .device
                    .update_descriptor_sets(&dsw, &[])
            };
        }

        self.occupancy += 1;
    }

    pub fn draw(&self) {
        self.context
            .borrow()
            .cmd_bind_graphics_pipeline(self.pipeline);

        self.context.borrow().cmd_bind_descriptor_sets(
            self.pipeline_layout,
            0,
            &[self.ubo_desc_set, self.mat_desc_set],
        );

        self.context
            .borrow()
            .cmd_set_push_constants(self.pipeline_layout, &self.push_constant_data);

        let viewport = vk::Viewport::default()
            .x(0.0)
            .y(0.0)
            .width(self.ci.window_width)
            .height(self.ci.window_height)
            .min_depth(0.0)
            .max_depth(1.0);
        self.context.borrow().cmd_set_viewport(viewport);

        let scissor = vk::Rect2D::default()
            .offset(vk::Offset2D { x: 0, y: 0 })
            .extent(vk::Extent2D {
                width: self.ci.window_width as u32,
                height: self.ci.window_height as u32,
            });
        self.context.borrow().cmd_set_scissor(scissor);

        //self.context.borrow().cmd_draw(6, self.occupancy, 0, 0);
        self.context.borrow().cmd_draw(6, self.occupancy, 0, 0);
    }
}

impl Drop for SpriteDatabase {
    fn drop(&mut self) {
        log::debug!("Destroying sprite database");

        unsafe {
            self.context
                .borrow()
                .device
                .destroy_image_view(self.default_image.image_view, None);
            self.context.borrow().memory_allocator.destroy_image(
                self.default_image.image,
                &mut self.default_image.image_memory,
            )
        };

        self.context.borrow().destroy_pipeline(self.pipeline);

        unsafe {
            self.context
                .borrow()
                .device
                .destroy_pipeline_layout(self.pipeline_layout, None);

            self.context
                .borrow()
                .device
                .destroy_descriptor_set_layout(self.mat_desc_set_layout, None);

            self.context
                .borrow()
                .device
                .destroy_descriptor_set_layout(self.ubo_desc_set_layout, None);

            self.context
                .borrow()
                .device
                .destroy_descriptor_pool(self.desc_pool, None);

            self.context
                .borrow()
                .device
                .destroy_sampler(self.texture_sampler, None);
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
