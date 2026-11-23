use super::{
    DEPTH_FORMAT, FRAMES_IN_FLIGHT, VulkanContext, buffer::Buffer,
    ctx_graphics_pipeline::GraphicsPipelineBuilder,
    render_common::{letterbox, make_ortho_projection, make_view},
};
use crate::errors::VulkanError;
use ash::vk::{self, DescriptorType};
use skrifa::FontRef;
use std::{cell::RefCell, mem::size_of, sync::Arc};
use vk_mem::{self, Alloc};

use surge_sdk::CreateInfo;

mod atlas_allocator;
use atlas_allocator::ShelfAllocator;

/// Side length of the (square) glyph atlas, in texels. One fixed `R8_UNORM` page (1 MiB).
/// Player-internal budget, not module-configurable (design §6).
const ATLAS_SIZE: u32 = 1024;

/// Transparent margin reserved around every packed glyph so bilinear sampling of a GPU-scaled
/// glyph never bleeds into a neighbour.
const ATLAS_GUTTER: u32 = 1;

/// Maximum glyph quads drawn in a single frame (sizes each per-frame instance SSBO).
const MAX_GLYPH_INSTANCES: u32 = 4096;

/// Maximum distinct fonts the database will hold. A sanity cap, not a GPU limit (there is one
/// shared atlas, not a per-font descriptor array).
const MAX_FONTS: usize = 16;

/// Data shared across all glyph instances in a frame. Provided via UBO. Identical layout to the
/// sprite database's `FrameGlobals` (std140: two mat4).
#[repr(C)]
struct FrameGlobals {
    _view: nalgebra::Matrix4<f32>,
    _proj: nalgebra::Matrix4<f32>,
}

const FRAME_GLOBALS_STRUCT_SIZE: u64 = size_of::<FrameGlobals>() as u64;

/// Per-glyph instance data (SSBO, std430, 16-byte aligned). One entry per drawn glyph quad,
/// indexed by `gl_InstanceIndex`. Unlike the sprite record there is no `material` field: every
/// glyph samples the one shared atlas, so only the atlas-cell UVs (`subtexture_data`) vary.
#[repr(C, align(16))]
#[allow(dead_code)] // fields are populated by add_text_run in Step 5; here only its size/align matter.
struct InstanceRecord {
    _model: nalgebra::Matrix4<f32>,
    _color: nalgebra::Vector4<f32>,
    _subtexture_data: nalgebra::Vector4<f32>,
}

const INSTANCE_RECORD_STRUCT_SIZE: u64 = size_of::<InstanceRecord>() as u64;

#[repr(C)]
#[derive(Debug, Clone, Copy)]
struct PushConstants {
    instance_records_ssbo_address: vk::DeviceAddress,
}

const PUSH_CONSTANTS_STRUCT_SIZE: u32 = size_of::<PushConstants>() as u32;

/// A font parsed once on the player side. Holds the one byte buffer per file (the parse-once-per-
/// side rule, §3.2); skrifa borrows these bytes to draw outlines, so the `FontRef` is rebuilt on
/// demand rather than stored self-referentially.
struct LoadedFont {
    #[allow(dead_code)] // outlines are drawn from these bytes in Step 5.
    bytes: Vec<u8>,
}

/// The player-side glyph renderer: a mutable `R8_UNORM` atlas plus an instanced-quad pipeline,
/// mirroring `SpriteDatabase` but with a single atlas image instead of a bindless texture array.
/// This is the Step 4 skeleton: it builds all GPU objects and draws zero instances. Rasterization,
/// the glyph cache, atlas uploads and `add_text_run` arrive in Step 5.
pub struct FontDatabase {
    context: Arc<RefCell<VulkanContext>>,

    ci: CreateInfo,

    /// Number of glyph instances queued for the current frame.
    occupancy: u32,

    _frame_globals_ubo: Buffer,

    instance_records_ssbos: [Buffer; FRAMES_IN_FLIGHT],
    instance_records_ssbo_addresses: [vk::DeviceAddress; FRAMES_IN_FLIGHT],

    /// Persistent, mutable coverage atlas (sub-region uploads in Step 5).
    atlas_image: vk::Image,
    atlas_allocation: vk_mem::Allocation,
    atlas_view: vk::ImageView,
    atlas_sampler: vk::Sampler,

    desc_pool: vk::DescriptorPool,
    ubo_desc_set_layout: vk::DescriptorSetLayout,
    atlas_desc_set_layout: vk::DescriptorSetLayout,
    ubo_desc_set: vk::DescriptorSet,
    atlas_desc_set: vk::DescriptorSet,

    pipeline_layout: vk::PipelineLayout,
    pipeline: vk::Pipeline,

    /// Fonts parsed on the player side, indexed by `FontId` (upload order; mirrors the module's).
    fonts: Vec<LoadedFont>,

    #[allow(dead_code)] // packs glyph bitmaps into the atlas in Step 5.
    allocator: ShelfAllocator,
}

impl FontDatabase {
    pub fn new(context: Arc<RefCell<VulkanContext>>, ci: CreateInfo) -> Result<Self, VulkanError> {
        log::info!("Creating font database");

        // Frame globals UBO
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

        // Per-frame instance record SSBOs
        let instance_records_ssbos: [Buffer; FRAMES_IN_FLIGHT] = (0..FRAMES_IN_FLIGHT)
            .map(|_| {
                let bci = vk::BufferCreateInfo::default()
                    .size(INSTANCE_RECORD_STRUCT_SIZE * (MAX_GLYPH_INSTANCES as u64))
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

                Buffer::new(context.clone(), &bci, &bai)
            })
            .collect::<Result<Vec<_>, _>>()?
            .try_into()
            .unwrap_or_else(|_| unreachable!("exactly FRAMES_IN_FLIGHT buffers were created"));

        let instance_records_ssbo_addresses: [vk::DeviceAddress; FRAMES_IN_FLIGHT] =
            std::array::from_fn(|i| {
                let ai = vk::BufferDeviceAddressInfo::default()
                    .buffer(instance_records_ssbos[i].get_buffer());
                unsafe { context.borrow().device.get_buffer_device_address(&ai) }
            });

        // Atlas image: persistent R8_UNORM, uploaded to incrementally (Step 5).
        let (atlas_image, atlas_allocation) = {
            let image_ci = vk::ImageCreateInfo {
                image_type: vk::ImageType::TYPE_2D,
                format: vk::Format::R8_UNORM,
                extent: vk::Extent3D {
                    width: ATLAS_SIZE,
                    height: ATLAS_SIZE,
                    depth: 1,
                },
                mip_levels: 1,
                array_layers: 1,
                samples: vk::SampleCountFlags::TYPE_1,
                tiling: vk::ImageTiling::OPTIMAL,
                usage: vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::SAMPLED,
                initial_layout: vk::ImageLayout::UNDEFINED,
                ..Default::default()
            };

            let alloc_ci = vk_mem::AllocationCreateInfo {
                usage: vk_mem::MemoryUsage::AutoPreferDevice,
                ..Default::default()
            };

            unsafe {
                context
                    .borrow()
                    .memory_allocator
                    .create_image(&image_ci, &alloc_ci)
                    .map_err(VulkanError::ImageCreationError)
            }
        }?;

        let atlas_srr = vk::ImageSubresourceRange {
            aspect_mask: vk::ImageAspectFlags::COLOR,
            base_mip_level: 0,
            level_count: 1,
            base_array_layer: 0,
            layer_count: 1,
        };

        let atlas_view = {
            let ivci = vk::ImageViewCreateInfo {
                view_type: vk::ImageViewType::TYPE_2D,
                format: vk::Format::R8_UNORM,
                subresource_range: atlas_srr,
                image: atlas_image,
                ..Default::default()
            };

            unsafe {
                context
                    .borrow()
                    .device
                    .create_image_view(&ivci, None)
                    .map_err(VulkanError::ImageViewCreationError)
            }
        }?;

        // Bring the fresh atlas into SHADER_READ_ONLY_OPTIMAL so it is a valid sampled image from
        // the first draw. Its contents are undefined, but Step 4 draws zero glyphs so nothing
        // samples it; Step 5's sub-region uploads transition back and forth from this layout.
        {
            context.borrow().cmd_immediate_begin()?;
            let barrier = [vk::ImageMemoryBarrier2::default()
                .src_stage_mask(vk::PipelineStageFlags2::TOP_OF_PIPE)
                .dst_stage_mask(vk::PipelineStageFlags2::FRAGMENT_SHADER)
                .dst_access_mask(vk::AccessFlags2::SHADER_READ)
                .old_layout(vk::ImageLayout::UNDEFINED)
                .new_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
                .image(atlas_image)
                .subresource_range(atlas_srr)];
            let dep_info = vk::DependencyInfo::default().image_memory_barriers(&barrier);
            let cmd = context.borrow().immediate_command_buffer;
            unsafe { context.borrow().device.cmd_pipeline_barrier2(cmd, &dep_info) };
            context.borrow().cmd_immediate_end()?;
            context.borrow().cmd_immediate_submit()?;
        }

        // Atlas sampler: LINEAR filtering so GPU-scaled glyphs interpolate; clamp to edge since a
        // glyph's UVs stay within its gutter-padded cell.
        let atlas_sampler = {
            let sci = vk::SamplerCreateInfo::default()
                .min_filter(vk::Filter::LINEAR)
                .mag_filter(vk::Filter::LINEAR)
                .address_mode_u(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                .address_mode_v(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                .address_mode_w(vk::SamplerAddressMode::CLAMP_TO_EDGE);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_sampler(&sci, None)
                    .map_err(VulkanError::SamplerCreationError)
            }
        }?;

        // Descriptor pool: one UBO, one sampler, one sampled image.
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
                    .descriptor_count(1),
            ];

            let dpci = vk::DescriptorPoolCreateInfo::default()
                .max_sets(2)
                .pool_sizes(&sizes);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_descriptor_pool(&dpci, None)
                    .map_err(VulkanError::DescriptorPoolCreationError)
            }
        }?;

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
                    .map_err(VulkanError::DescriptorSetLayoutCreationError)
            }
        }?;

        // Single atlas: a plain sampler + one sampled image, no bindless array.
        let atlas_desc_set_layout = {
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
                    descriptor_count: 1,
                    stage_flags: vk::ShaderStageFlags::FRAGMENT,
                    ..Default::default()
                },
            ];

            let dslci = vk::DescriptorSetLayoutCreateInfo::default().bindings(&bindings);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_descriptor_set_layout(&dslci, None)
                    .map_err(VulkanError::DescriptorSetLayoutCreationError)
            }
        }?;

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
                    .map_err(VulkanError::DescriptorSetAllocationError)
            }
        }?[0];

        let atlas_desc_set = {
            let layouts = [atlas_desc_set_layout];
            let ai = vk::DescriptorSetAllocateInfo::default()
                .descriptor_pool(desc_pool)
                .set_layouts(&layouts);

            unsafe {
                context
                    .borrow()
                    .device
                    .allocate_descriptor_sets(&ai)
                    .map_err(VulkanError::DescriptorSetAllocationError)
            }
        }?[0];

        // Frame globals UBO contents + descriptor
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

            let view = make_view(nalgebra::Vector2::from_element(0.0f32));
            let proj = make_ortho_projection(ci.window_width, ci.window_height);

            unsafe {
                *frame_globals_ubo.mapped_ptr::<FrameGlobals>() = FrameGlobals {
                    _view: view,
                    _proj: proj,
                };
            }

            frame_globals_ubo.flush()?;

            unsafe { context.borrow().device.update_descriptor_sets(&dsw, &[]) };
        }

        // Sampler + atlas image descriptor
        {
            let si = [vk::DescriptorImageInfo::default().sampler(atlas_sampler)];
            let ii = [vk::DescriptorImageInfo::default()
                .image_view(atlas_view)
                .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)];

            let dsw = [
                vk::WriteDescriptorSet::default()
                    .dst_set(atlas_desc_set)
                    .dst_binding(0)
                    .descriptor_type(vk::DescriptorType::SAMPLER)
                    .descriptor_count(1)
                    .image_info(&si),
                vk::WriteDescriptorSet::default()
                    .dst_set(atlas_desc_set)
                    .dst_binding(1)
                    .descriptor_type(vk::DescriptorType::SAMPLED_IMAGE)
                    .descriptor_count(1)
                    .image_info(&ii),
            ];

            unsafe { context.borrow().device.update_descriptor_sets(&dsw, &[]) };
        }

        let pipeline_layout = {
            let pcr = [vk::PushConstantRange::default()
                .offset(0)
                .size(PUSH_CONSTANTS_STRUCT_SIZE)
                .stage_flags(vk::ShaderStageFlags::VERTEX)];

            let set_layouts = [ubo_desc_set_layout, atlas_desc_set_layout];

            let pci = vk::PipelineLayoutCreateInfo::default()
                .push_constant_ranges(&pcr)
                .set_layouts(&set_layouts);

            unsafe {
                context
                    .borrow()
                    .device
                    .create_pipeline_layout(&pci, None)
                    .map_err(VulkanError::GraphicsPipelineCreationError)
            }
        }?;

        let pipeline = {
            static FONT_VERT_SPV: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/font_vert.spv"));
            static FONT_FRAG_SPV: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/font_frag.spv"));

            let vert_shader = context.borrow().load_shader_module(FONT_VERT_SPV)?;
            let frag_shader = context.borrow().load_shader_module(FONT_FRAG_SPV)?;

            // Text is always alpha-blended coverage; the depth test matches the sprite pipeline so
            // the two share the depth buffer (text drawn second wins equal-z ties).
            let pb = GraphicsPipelineBuilder::default()
                .set_layout(pipeline_layout)
                .set_shaders(vert_shader, frag_shader, c"main", c"main")
                .set_input_topology(vk::PrimitiveTopology::TRIANGLE_LIST)
                .set_polygon_mode(vk::PolygonMode::FILL)
                .set_cull_mode(vk::CullModeFlags::NONE, vk::FrontFace::CLOCKWISE)
                .set_multisampling_none()
                .set_depth_test_enabled(true, vk::CompareOp::GREATER_OR_EQUAL)
                .set_color_attachment_format(context.borrow().swapchain_data.format)
                .set_depth_format(DEPTH_FORMAT)
                .set_blending_alpha();

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
            atlas_image,
            atlas_allocation,
            atlas_view,
            atlas_sampler,
            desc_pool,
            ubo_desc_set_layout,
            atlas_desc_set_layout,
            ubo_desc_set,
            atlas_desc_set,
            pipeline_layout,
            pipeline,
            fonts: Vec::new(),
            allocator: ShelfAllocator::new(ATLAS_SIZE, ATLAS_SIZE, ATLAS_GUTTER),
        })
    }

    /// Reads and parses a font file once into one byte buffer and one skrifa font (validated
    /// here), returning its `FontId` (upload order; matches the module side). Outline drawing and
    /// rasterization from these bytes arrive in Step 5.
    pub fn load_font(&mut self, path: &str) -> Result<usize, VulkanError> {
        if self.fonts.len() == MAX_FONTS {
            return Err(VulkanError::FontDatabaseFull);
        }

        let bytes = std::fs::read(path).map_err(VulkanError::FontFileIOError)?;

        // Validate the file parses now so a bad font fails at load, not mid-frame. The borrowed
        // FontRef cannot be stored alongside its bytes, so it is dropped and rebuilt on demand.
        FontRef::new(&bytes).map_err(|e| VulkanError::FontParseError(e.to_string()))?;

        let font_id = self.fonts.len();
        self.fonts.push(LoadedFont { bytes });

        Ok(font_id)
    }

    pub fn draw(&mut self) -> Result<(), VulkanError> {
        self.context
            .borrow()
            .cmd_bind_graphics_pipeline(self.pipeline);

        self.context.borrow().cmd_bind_descriptor_sets(
            self.pipeline_layout,
            0,
            &[self.ubo_desc_set, self.atlas_desc_set],
        );

        let current_frame = self.context.borrow().current_frame;

        self.instance_records_ssbos[current_frame].flush()?;

        let push_constant_data = PushConstants {
            instance_records_ssbo_address: self.instance_records_ssbo_addresses[current_frame],
        };

        self.context
            .borrow()
            .cmd_set_push_constants(self.pipeline_layout, &push_constant_data);

        // Share the sprite pipeline's letterboxed viewport: design resolution into the swapchain,
        // aspect preserved.
        let target = self.context.borrow().swapchain_extent();
        let (x, y, width, height) = letterbox(self.ci.window_width, self.ci.window_height, target);

        let viewport = vk::Viewport::default()
            .x(x as f32)
            .y(y as f32)
            .width(width as f32)
            .height(height as f32)
            .min_depth(0.0)
            .max_depth(1.0);
        self.context.borrow().cmd_set_viewport(viewport);

        let scissor = vk::Rect2D::default()
            .offset(vk::Offset2D { x, y })
            .extent(vk::Extent2D { width, height });
        self.context.borrow().cmd_set_scissor(scissor);

        self.context.borrow().cmd_draw(6, self.occupancy, 0, 0);

        self.occupancy = 0;

        Ok(())
    }

    pub fn get_create_info(&self) -> CreateInfo {
        self.ci
    }
}

impl Drop for FontDatabase {
    fn drop(&mut self) {
        log::debug!("Destroying font database");

        self.context.borrow().destroy_pipeline(self.pipeline);

        unsafe {
            let ctx = self.context.borrow();

            ctx.device
                .destroy_pipeline_layout(self.pipeline_layout, None);
            ctx.device
                .destroy_descriptor_set_layout(self.atlas_desc_set_layout, None);
            ctx.device
                .destroy_descriptor_set_layout(self.ubo_desc_set_layout, None);
            ctx.device.destroy_descriptor_pool(self.desc_pool, None);
            ctx.device.destroy_sampler(self.atlas_sampler, None);
            ctx.device.destroy_image_view(self.atlas_view, None);
            ctx.memory_allocator
                .destroy_image(self.atlas_image, &mut self.atlas_allocation);
        }
    }
}

#[cfg(test)]
#[path = "tests.rs"]
mod tests;
