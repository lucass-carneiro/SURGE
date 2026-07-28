use super::{
    DPETH_FORMAT, FRAMES_IN_FLIGHT, VulkanContext, buffer::Buffer,
    ctx_graphics_pipeline::GraphicsPipelineBuilder, texture::Texture,
};
use crate::errors::VulkanError;
use ash::vk::{self, DescriptorType};
use nalgebra;
use png;
use std::{cell::RefCell, fs::File, io::BufReader, mem::size_of, slice, sync::Arc};
use vk_mem;

/// Database blending mode
#[derive(Debug, Clone, Copy)]
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

/// Texture filtering modes
#[derive(Debug, Clone, Copy)]
pub enum TextureFilteringMode {
    Nearest,
    Linear,
}

/// Texture anisotropic  filtering level
#[derive(Debug, Clone, Copy)]
pub enum TextureFilteringLevel {
    X0 = 0,
    X1 = 1,
    X2 = 2,
    X4 = 4,
    X8 = 8,
    X16 = 16,
}

/// Controls database creation
#[derive(Debug, Clone, Copy)]
pub struct CreateInfo {
    pub blending_mode: BlendingMode,
    pub texture_filtering_mode: TextureFilteringMode,
    pub texture_filtering_level: TextureFilteringLevel,
    pub max_sprites: u32,
    pub window_width: f32,
    pub window_height: f32,
}

#[derive(Debug)]
pub struct SubTextureInfo {
    pub origin: nalgebra::Vector2<u32>,
    pub extent: nalgebra::Vector2<u32>,
}

impl SubTextureInfo {
    pub fn default() -> Self {
        SubTextureInfo {
            origin: nalgebra::Vector2::from_element(0),
            extent: nalgebra::Vector2::from_element(0),
        }
    }

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
#[derive(Debug)]
pub struct InstanceInfo {
    pub position: nalgebra::Vector2<f32>,
    pub scale: nalgebra::Vector2<f32>,
    pub z: f32,
    pub texture_id: usize,
    pub color_multiplier: nalgebra::Vector4<f32>,
    pub subtexture_info: Option<SubTextureInfo>,
}

/// Data shared across all sprite instances in a frame.
/// Provided via UBO.
struct FrameGlobals {
    _view: nalgebra::Matrix4<f32>,
    _proj: nalgebra::Matrix4<f32>,
}

const FRAME_GLOBALS_STRUCT_SIZE: u64 = size_of::<FrameGlobals>() as u64;

/// Data that describes each sprite instance
/// Provided via SSBO (needs to be 16 byte aligned)
/// One entry per sprite
/// Indexed by gl_InstanceID (or equivalent)
#[repr(align(16))]
struct InstanceRecord {
    _model: nalgebra::Matrix4<f32>,
    _color: nalgebra::Vector4<f32>,
    _subtexture_data: nalgebra::Vector4<f32>,
    _material_id: u32,
}

const INSTANCE_RECORD_STRUCT_SIZE: u64 = size_of::<InstanceRecord>() as u64;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
struct PushConstants {
    instance_records_ssbo_address: vk::DeviceAddress,
}

const PUSH_CONSTANTS_STRUCT_SIZE: u32 = size_of::<PushConstants>() as u32;

pub struct SpriteDatabase {
    /// The vulkan context that created this database
    context: Arc<RefCell<VulkanContext>>,

    /// Creation info
    ci: CreateInfo,

    /// The number of elements currently in the databasae
    occupancy: u32,

    /// UBO storing frame global data (see FrameGlobals)
    _frame_globals_ubo: Buffer,

    /// SSBO storing instance data (see InstanceRecord)
    /// We need to make sure that we don't write to a buffer
    /// while it is being read by the GPU. For that reason,
    /// we need as many buffers as there are frames in flight
    instance_records_ssbos: [Buffer; FRAMES_IN_FLIGHT],

    /// Device addresses of instance record SSBOs
    instance_records_ssbo_addresses: [vk::DeviceAddress; FRAMES_IN_FLIGHT],

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

    /// Sprite textures uploaded to GPU memory
    uploaded_textures: Vec<Texture>,
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

            Buffer::new(context.clone(), &bci, &bai)
        }?;

        //Instance record SSBOs
        let instance_records_ssbos: [Buffer; FRAMES_IN_FLIGHT] = std::array::from_fn(|_| {
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

            Buffer::new(context.clone(), &bci, &bai).unwrap()
        });

        let instance_records_ssbo_addresses: [vk::DeviceAddress; FRAMES_IN_FLIGHT] =
            std::array::from_fn(|i| {
                let ai = vk::BufferDeviceAddressInfo::default()
                    .buffer(instance_records_ssbos[i].get_buffer());
                unsafe { context.borrow().device.get_buffer_device_address(&ai) }
            });

        // Texture sampler
        // TODO: Texture filtering and mip maps
        let texture_sampler = {
            let mut sci = vk::SamplerCreateInfo::default()
                .address_mode_u(vk::SamplerAddressMode::CLAMP_TO_BORDER)
                .address_mode_v(vk::SamplerAddressMode::CLAMP_TO_BORDER)
                .address_mode_w(vk::SamplerAddressMode::CLAMP_TO_BORDER)
                .border_color(vk::BorderColor::FLOAT_TRANSPARENT_BLACK)
                .anisotropy_enable(true);

            sci = match ci.texture_filtering_mode {
                TextureFilteringMode::Nearest => sci
                    .min_filter(vk::Filter::NEAREST)
                    .mag_filter(vk::Filter::NEAREST),
                TextureFilteringMode::Linear => sci
                    .min_filter(vk::Filter::LINEAR)
                    .mag_filter(vk::Filter::LINEAR),
            };

            sci = match ci.texture_filtering_level {
                TextureFilteringLevel::X0 => sci.anisotropy_enable(false),
                TextureFilteringLevel::X1 => sci.max_anisotropy(1.0),
                TextureFilteringLevel::X2 => sci.max_anisotropy(2.0),
                TextureFilteringLevel::X4 => sci.max_anisotropy(4.0),
                TextureFilteringLevel::X8 => sci.max_anisotropy(8.0),
                TextureFilteringLevel::X16 => sci.max_anisotropy(16.0),
            };

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
                .buffer(frame_globals_ubo.get_buffer())
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
                *(frame_globals_ubo.get_allocation_info().mapped_data as *mut FrameGlobals) =
                    FrameGlobals {
                        _view: view,
                        _proj: proj,
                    };
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
            static SPRITE_VERT_SPV: &[u8] =
                include_bytes!(concat!(env!("OUT_DIR"), "/sprite_vert.spv"));
            static SPRITE_FRAG_SPV: &[u8] =
                include_bytes!(concat!(env!("OUT_DIR"), "/sprite_frag.spv"));

            let vert_shader = context.borrow().load_shader_module(SPRITE_VERT_SPV)?;
            let frag_shader = context.borrow().load_shader_module(SPRITE_FRAG_SPV)?;

            let mut pb = GraphicsPipelineBuilder::default()
                .set_layout(pipeline_layout)
                .set_shaders(vert_shader, frag_shader, c"main", c"main")
                .set_input_topology(vk::PrimitiveTopology::TRIANGLE_LIST)
                .set_polygon_mode(vk::PolygonMode::FILL)
                .set_cull_mode(vk::CullModeFlags::NONE, vk::FrontFace::CLOCKWISE)
                .set_multisampling_none() // TODO: Enable multisampling
                .set_depth_test_enabled(true, vk::CompareOp::GREATER_OR_EQUAL)
                .set_color_attachment_format(context.borrow().swapchain_data.format)
                .set_depth_format(DPETH_FORMAT);

            pb = match ci.blending_mode {
                BlendingMode::None => pb.set_blending_none(),
                BlendingMode::Additive => pb.set_blending_additive(),
                BlendingMode::Alpha => pb.set_blending_alpha(),
            };

            let pipeline = context.borrow().create_graphics_pipeline(pb)?;

            context.borrow().unload_shader_module(frag_shader);
            context.borrow().unload_shader_module(vert_shader);

            pipeline
        };

        Ok(Self {
            context,
            ci,
            occupancy: 0,
            _frame_globals_ubo: frame_globals_ubo,
            instance_records_ssbos,
            instance_records_ssbo_addresses,
            texture_sampler,
            desc_pool,
            ubo_desc_set_layout,
            mat_desc_set_layout,
            ubo_desc_set,
            mat_desc_set,
            pipeline_layout,
            pipeline,
            uploaded_textures: Vec::new(),
        })
    }

    /// Adds a sprite instance to the databse
    pub fn add_instance(&mut self, aii: &InstanceInfo) {
        // Check if the database can fit a new instance
        if self.occupancy == self.ci.max_sprites {
            log::warn!("Unable to add new sprite instance to full database. Ignoring request");
            return;
        }

        // If we are adding a subtexture instance, make sure that
        // the subtexture can fit inside the main texture.
        let subtexture_data = match &aii.subtexture_info {
            Some(sti) => {
                let o_extent = self.uploaded_textures[aii.texture_id].get_image_extent();
                let ow = o_extent.width;
                let oh = o_extent.height;

                let sx = sti.origin[0];
                let sy = sti.origin[1];
                let sw = sti.extent[0];
                let sh = sti.extent[1];

                let x_fits = sx < ow && (sx + sw) < ow;
                let y_fits = sy < oh && (sy + sh) < oh;
                let subimage_fits = x_fits && y_fits;

                if !subimage_fits {
                    log::warn!(
                        "Requested subtexture is not contained within the main texture. Drawing the whole texture instead."
                    );

                    nalgebra::Vector4::new(1.0f32, 1.0f32, 0.0f32, 0.0f32)
                } else {
                    nalgebra::Vector4::new(
                        (sw as f32) / (ow as f32),
                        (sh as f32) / (oh as f32),
                        (sx as f32) / (ow as f32),
                        (sy as f32) / (oh as f32),
                    )
                }
            }
            None => nalgebra::Vector4::new(1.0f32, 1.0f32, 0.0f32, 0.0f32),
        };

        let current_frame = self.context.borrow().current_frame;

        let instance_records_slice: &mut [InstanceRecord] = unsafe {
            slice::from_raw_parts_mut(
                self.instance_records_ssbos[current_frame]
                    .get_allocation_info()
                    .mapped_data as *mut InstanceRecord,
                self.ci.max_sprites as usize,
            )
        };

        let record = InstanceRecord {
            _model: make_model_matrix(aii.position, aii.scale, aii.z),
            _color: aii.color_multiplier,
            _subtexture_data: subtexture_data,
            _material_id: aii.texture_id as u32,
        };

        instance_records_slice[self.occupancy as usize] = record;

        self.occupancy += 1;
    }

    pub fn upload_texture(&mut self, texture_file: &str) -> Result<(), VulkanError> {
        // Only do this if the ammount of stored image records < database capacity
        if self.uploaded_textures.len() == self.ci.max_sprites as usize {
            log::warn!("Unable to upload new texture to full database. Ignoring request");
            return Ok(());
        }

        // Read image file and metadata
        let file = File::open(texture_file).map_err(|e| VulkanError::TextureIOError(e))?;
        let buf_reader = BufReader::new(file);

        let mut decoder = png::Decoder::new(buf_reader);
        // Ensure output is always RGBA regardless of source format
        decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::ALPHA);
        let mut reader = decoder
            .read_info()
            .map_err(|e| VulkanError::TextureDecodingError(e))?;

        let image_size = reader.output_buffer_size().unwrap();

        // Create CPU buffer
        let cpu_buffer = {
            let bci = vk::BufferCreateInfo::default()
                .size(image_size as u64)
                .usage(vk::BufferUsageFlags::TRANSFER_SRC);

            let bai = vk_mem::AllocationCreateInfo {
                flags: vk_mem::AllocationCreateFlags::MAPPED
                    | vk_mem::AllocationCreateFlags::HOST_ACCESS_SEQUENTIAL_WRITE,
                usage: vk_mem::MemoryUsage::Auto,
                ..Default::default()
            };

            Buffer::new(self.context.clone(), &bci, &bai)
        }?;

        let cpu_buffer_slice: &mut [u8] = unsafe {
            slice::from_raw_parts_mut(
                cpu_buffer.get_allocation_info().mapped_data as *mut u8,
                image_size,
            )
        };

        // Read image into CPU buffer
        let read_info = reader
            .next_frame(cpu_buffer_slice)
            .map_err(|e| VulkanError::TextureDecodingError(e))?;

        assert!(read_info.buffer_size() == image_size);

        // Allocate GPU texture
        let format = match read_info.bit_depth {
            png::BitDepth::Eight => vk::Format::R8G8B8A8_UNORM,
            png::BitDepth::Sixteen => vk::Format::R16G16B16A16_UNORM,
            other => return Err(VulkanError::UnsupportedTextureBitDepth(other)),
        };

        let texture = Texture::new(
            self.context.clone(),
            read_info.width,
            read_info.height,
            format,
        )?;

        // Submit CPU to GPU buffer copy command
        texture.immediate_upload_from_buffer(&cpu_buffer)?;

        // Add GPU image record to database
        self.uploaded_textures.push(texture);

        // Update texture descriptor.
        // TODO: Do this somewhere else?
        {
            let mut si = Vec::new();

            for texture in &self.uploaded_textures {
                si.push(
                    vk::DescriptorImageInfo::default()
                        .image_view(texture.get_image_view())
                        .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL),
                )
            }

            let dsw = [vk::WriteDescriptorSet::default()
                .dst_set(self.mat_desc_set)
                .dst_binding(1)
                .descriptor_type(vk::DescriptorType::SAMPLED_IMAGE)
                .descriptor_count(si.len() as u32)
                .image_info(&si)];

            unsafe {
                self.context
                    .borrow()
                    .device
                    .update_descriptor_sets(&dsw, &[])
            };
        }

        Ok(())
    }

    pub fn draw(&mut self) {
        self.context
            .borrow()
            .cmd_bind_graphics_pipeline(self.pipeline);

        self.context.borrow().cmd_bind_descriptor_sets(
            self.pipeline_layout,
            0,
            &[self.ubo_desc_set, self.mat_desc_set],
        );

        let current_frame = self.context.borrow().current_frame;
        let push_constant_data = PushConstants {
            instance_records_ssbo_address: self.instance_records_ssbo_addresses[current_frame],
        };

        self.context
            .borrow()
            .cmd_set_push_constants(self.pipeline_layout, &push_constant_data);

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

        self.context.borrow().cmd_draw(6, self.occupancy, 0, 0);

        self.occupancy = 0;
    }

    pub fn get_create_info(&self) -> CreateInfo {
        self.ci
    }
}

impl Drop for SpriteDatabase {
    fn drop(&mut self) {
        log::debug!("Destroying sprite database");

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
    }
}

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

/// Crete a model matrix for a given sprite
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

#[cfg(test)]
mod tests {
    use super::*;

    /// Runs a sprite's z through the same proj * view * model chain
    /// the engine builds at draw time and returns the resulting
    /// Vulkan NDC/depth z (post perspective-divide).
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
}
