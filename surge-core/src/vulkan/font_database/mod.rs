use super::{
    DEPTH_FORMAT, FRAMES_IN_FLIGHT, VulkanContext,
    buffer::Buffer,
    ctx_graphics_pipeline::GraphicsPipelineBuilder,
    render_common::{letterbox, make_model_matrix, make_ortho_projection, make_view},
};
use crate::errors::VulkanError;
use ash::vk::{self, DescriptorType};
use skrifa::FontRef;
use std::{cell::RefCell, mem::size_of, rc::Rc, slice};
use vk_mem::{self, Alloc};

use surge_sdk::{CreateInfo, TextRun};

mod atlas_allocator;
mod glyph_cache;
mod rasterizer;
use atlas_allocator::{AtlasRect, ShelfAllocator};
use glyph_cache::{GlyphCache, GlyphKey, glyph_cell_uv, glyph_quad, px_bucket};
use rasterizer::rasterize_glyph;

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
    bytes: Vec<u8>,
}

/// The player-side glyph renderer: a mutable `R8_UNORM` atlas plus an instanced-quad pipeline,
/// mirroring `SpriteDatabase` but with a single atlas image instead of a bindless texture array.
/// This is the Step 4 skeleton: it builds all GPU objects and draws zero instances. Rasterization,
/// the glyph cache, atlas uploads and `add_text_run` arrive in Step 5.
pub struct FontDatabase {
    context: Rc<RefCell<VulkanContext>>,

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

    /// Rasterized-glyph cache over the shelf atlas allocator (keying, packing, sentinels).
    glyph_cache: GlyphCache,

    /// Staging buffers for glyph bitmaps rasterized this frame, each with its destination atlas
    /// cell, flushed into the atlas at the next `draw`. Kept alive until the copy has completed.
    pending_glyph_uploads: Vec<(Buffer, AtlasRect)>,
}

impl FontDatabase {
    pub fn new(context: Rc<RefCell<VulkanContext>>, ci: CreateInfo) -> Result<Self, VulkanError> {
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
            unsafe {
                context
                    .borrow()
                    .device
                    .cmd_pipeline_barrier2(cmd, &dep_info)
            };
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
            static FONT_VERT_SPV: &[u8] =
                include_bytes!(concat!(env!("OUT_DIR"), "/font_vert.spv"));
            static FONT_FRAG_SPV: &[u8] =
                include_bytes!(concat!(env!("OUT_DIR"), "/font_frag.spv"));

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
            glyph_cache: GlyphCache::new(ShelfAllocator::new(ATLAS_SIZE, ATLAS_SIZE, ATLAS_GUTTER)),
            pending_glyph_uploads: Vec::new(),
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

    /// Queues one shaped run for drawing: rasterizes and caches each glyph (packing new bitmaps
    /// into the atlas), then appends a textured-quad instance per visible glyph. Glyphs that
    /// rasterize empty or overflow the atlas draw nothing. The actual atlas writes are recorded and
    /// submitted by the next `draw` (see `flush_glyph_uploads`).
    pub fn add_text_run(&mut self, run: &TextRun) {
        if run.font_id >= self.fonts.len() {
            log::warn!(
                "text run references font_id {} but only {} font(s) are loaded; ignoring run",
                run.font_id,
                self.fonts.len()
            );
            return;
        }

        let px = px_bucket(run.px_size);

        for glyph in &run.glyphs {
            if self.occupancy == MAX_GLYPH_INSTANCES {
                log::warn!(
                    "font instance buffer full ({MAX_GLYPH_INSTANCES} glyphs this frame); dropping the rest of the run"
                );
                return;
            }

            let key = GlyphKey {
                font_id: run.font_id,
                glyph_id: glyph.glyph_id,
                px,
            };

            // Split borrow: the rasterize closure reads `self.fonts`, `get_or_insert` mutates
            // `self.glyph_cache` — disjoint fields, so the closure runs only on a cache miss.
            let fonts = &self.fonts;
            let (entry, upload) = self.glyph_cache.get_or_insert(key, || {
                rasterize_glyph(&fonts[run.font_id].bytes, glyph.glyph_id, px as f32)
            });

            if entry.draws_nothing() {
                continue;
            }

            if let Some((cell, coverage)) = upload
                && let Err(e) = self.stage_glyph_upload(&coverage, cell)
            {
                // Staging the bitmap failed (e.g. OOM): drop the glyph so its reserved but
                // never-written cell is never sampled, and never re-rasterize it.
                log::error!(
                    "failed to stage glyph upload: {e}; dropping glyph {}",
                    glyph.glyph_id
                );
                self.glyph_cache.mark_dropped(key);
                continue;
            }

            self.emit_glyph_instance(&entry, glyph.x, glyph.y, run);
        }
    }

    /// Writes one glyph quad into the current frame's instance SSBO at the next free slot.
    fn emit_glyph_instance(
        &mut self,
        entry: &glyph_cache::GlyphEntry,
        pen_x: f32,
        pen_y: f32,
        run: &TextRun,
    ) {
        let (position, scale) = glyph_quad(pen_x, pen_y, entry);

        let record = InstanceRecord {
            _model: make_model_matrix(position, scale, run.z),
            _color: run.color,
            _subtexture_data: glyph_cell_uv(entry.cell, ATLAS_SIZE),
        };

        let current_frame = self.context.borrow().current_frame;
        let instance_records_slice: &mut [InstanceRecord] = unsafe {
            slice::from_raw_parts_mut(
                self.instance_records_ssbos[current_frame].mapped_ptr::<InstanceRecord>(),
                MAX_GLYPH_INSTANCES as usize,
            )
        };

        instance_records_slice[self.occupancy as usize] = record;
        self.occupancy += 1;
    }

    /// Copies a freshly rasterized coverage bitmap into a host-visible staging buffer and records
    /// it, with its destination atlas cell, for the next `flush_glyph_uploads`.
    fn stage_glyph_upload(&mut self, coverage: &[u8], cell: AtlasRect) -> Result<(), VulkanError> {
        let bci = vk::BufferCreateInfo::default()
            .size(coverage.len() as u64)
            .usage(vk::BufferUsageFlags::TRANSFER_SRC);

        let bai = vk_mem::AllocationCreateInfo {
            flags: vk_mem::AllocationCreateFlags::MAPPED
                | vk_mem::AllocationCreateFlags::HOST_ACCESS_SEQUENTIAL_WRITE,
            usage: vk_mem::MemoryUsage::Auto,
            ..Default::default()
        };

        let buffer = Buffer::new(self.context.clone(), &bci, &bai)?;

        let staging: &mut [u8] =
            unsafe { slice::from_raw_parts_mut(buffer.mapped_ptr::<u8>(), coverage.len()) };
        staging.copy_from_slice(coverage);
        buffer.flush()?;

        self.pending_glyph_uploads.push((buffer, cell));
        Ok(())
    }

    /// Copies this frame's staged glyph bitmaps into the persistent atlas as sub-regions, then
    /// drains the device so the write is serialized against the `FRAMES_IN_FLIGHT` frames that may
    /// still be sampling the atlas (a write-after-read hazard on one persistent image — *not* the
    /// sprite DB's descriptor-rewrite drain, §3.1). No-op when nothing was rasterized this frame.
    fn flush_glyph_uploads(&mut self) -> Result<(), VulkanError> {
        if self.pending_glyph_uploads.is_empty() {
            return Ok(());
        }

        let atlas_srr = vk::ImageSubresourceRange {
            aspect_mask: vk::ImageAspectFlags::COLOR,
            base_mip_level: 0,
            level_count: 1,
            base_array_layer: 0,
            layer_count: 1,
        };

        self.context.borrow().cmd_immediate_begin()?;
        let cmd = self.context.borrow().immediate_command_buffer;

        // One transition for all of this frame's sub-region writes: SHADER_READ_ONLY -> TRANSFER_DST.
        let to_dst = [vk::ImageMemoryBarrier2::default()
            .src_stage_mask(vk::PipelineStageFlags2::FRAGMENT_SHADER)
            .src_access_mask(vk::AccessFlags2::SHADER_READ)
            .dst_stage_mask(vk::PipelineStageFlags2::TRANSFER)
            .dst_access_mask(vk::AccessFlags2::TRANSFER_WRITE)
            .old_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
            .new_layout(vk::ImageLayout::TRANSFER_DST_OPTIMAL)
            .image(self.atlas_image)
            .subresource_range(atlas_srr)];
        unsafe {
            self.context.borrow().device.cmd_pipeline_barrier2(
                cmd,
                &vk::DependencyInfo::default().image_memory_barriers(&to_dst),
            );
        }

        for (buffer, cell) in &self.pending_glyph_uploads {
            let sr_layers = vk::ImageSubresourceLayers::default()
                .aspect_mask(vk::ImageAspectFlags::COLOR)
                .mip_level(0)
                .base_array_layer(0)
                .layer_count(1);

            let region = [vk::BufferImageCopy::default()
                .buffer_offset(0)
                .buffer_row_length(0)
                .buffer_image_height(0)
                .image_subresource(sr_layers)
                .image_offset(vk::Offset3D {
                    x: cell.x as i32,
                    y: cell.y as i32,
                    z: 0,
                })
                .image_extent(vk::Extent3D {
                    width: cell.width,
                    height: cell.height,
                    depth: 1,
                })];

            unsafe {
                self.context.borrow().device.cmd_copy_buffer_to_image(
                    cmd,
                    buffer.get_buffer(),
                    self.atlas_image,
                    vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                    &region,
                );
            }
        }

        // Back to SHADER_READ_ONLY for sampling.
        let to_read = [vk::ImageMemoryBarrier2::default()
            .src_stage_mask(vk::PipelineStageFlags2::TRANSFER)
            .src_access_mask(vk::AccessFlags2::TRANSFER_WRITE)
            .dst_stage_mask(vk::PipelineStageFlags2::FRAGMENT_SHADER)
            .dst_access_mask(vk::AccessFlags2::SHADER_READ)
            .old_layout(vk::ImageLayout::TRANSFER_DST_OPTIMAL)
            .new_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
            .image(self.atlas_image)
            .subresource_range(atlas_srr)];
        unsafe {
            self.context.borrow().device.cmd_pipeline_barrier2(
                cmd,
                &vk::DependencyInfo::default().image_memory_barriers(&to_read),
            );
        }

        self.context.borrow().cmd_immediate_end()?;

        // Drain in-flight frames still sampling the atlas before the transfer write executes.
        unsafe {
            self.context
                .borrow()
                .device
                .device_wait_idle()
                .map_err(VulkanError::DeviceWaitIdleError)?;
        }

        self.context.borrow().cmd_immediate_submit()?;
        self.pending_glyph_uploads.clear();

        Ok(())
    }

    pub fn draw(&mut self) -> Result<(), VulkanError> {
        // Before binding/sampling the atlas: flush this frame's glyph uploads into it.
        self.flush_glyph_uploads()?;

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
