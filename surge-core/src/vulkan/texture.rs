use super::{VulkanContext, buffer::Buffer};
use crate::errors::VulkanError;
use ash::{self, vk};
use std::{cell::RefCell, sync::Arc};
use vk_mem::Alloc;

pub struct Texture {
    context: Arc<RefCell<VulkanContext>>,
    image: vk::Image,
    image_view: vk::ImageView,
    image_memory: vk_mem::Allocation,
    image_extent: vk::Extent3D,
    image_srr: vk::ImageSubresourceRange,
}

impl Texture {
    pub fn new(
        context: Arc<RefCell<VulkanContext>>,
        width: u32,
        height: u32,
        format: vk::Format,
    ) -> Result<Self, VulkanError> {
        let image_extent = vk::Extent3D {
            width,
            height,
            depth: 1,
        };

        let image_ci = vk::ImageCreateInfo {
            image_type: vk::ImageType::TYPE_2D,
            format: format,
            extent: image_extent,
            mip_levels: 1,
            array_layers: 1,
            samples: vk::SampleCountFlags::TYPE_1,
            tiling: vk::ImageTiling::OPTIMAL,
            usage: vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::SAMPLED,
            initial_layout: vk::ImageLayout::UNDEFINED,
            ..Default::default()
        };

        let alloc_ci = vk_mem::AllocationCreateInfo {
            usage: vk_mem::MemoryUsage::Auto,
            ..Default::default()
        };

        let (image, image_memory) = unsafe {
            context
                .borrow()
                .memory_allocator
                .create_image(&image_ci, &alloc_ci)
                .map_err(|e| VulkanError::DepthImageCreationError(e))
        }?;

        let image_srr = vk::ImageSubresourceRange {
            aspect_mask: vk::ImageAspectFlags::COLOR,
            base_mip_level: 0,
            level_count: 1,
            base_array_layer: 0,
            layer_count: 1,
            ..Default::default()
        };

        let ivci = vk::ImageViewCreateInfo {
            view_type: vk::ImageViewType::TYPE_2D,
            format: format,
            subresource_range: image_srr,
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

        Ok(Self {
            context,
            image,
            image_view,
            image_memory,
            image_extent,
            image_srr,
        })
    }

    pub fn get_image_view(&self) -> vk::ImageView {
        self.image_view
    }

    pub fn get_image_extent(&self) -> vk::Extent2D {
        vk::Extent2D::default()
            .width(self.image_extent.width)
            .height(self.image_extent.height)
    }

    pub fn immediate_upload_from_buffer(&self, src_buffer: &Buffer) -> Result<(), VulkanError> {
        self.context.borrow().cmd_immediate_begin()?;

        let barrier_1 = [vk::ImageMemoryBarrier2::default()
            .src_stage_mask(vk::PipelineStageFlags2::TOP_OF_PIPE)
            .dst_stage_mask(vk::PipelineStageFlags2::TRANSFER)
            .dst_access_mask(vk::AccessFlags2::TRANSFER_WRITE)
            .old_layout(vk::ImageLayout::UNDEFINED)
            .new_layout(vk::ImageLayout::TRANSFER_DST_OPTIMAL)
            .image(self.image)
            .subresource_range(self.image_srr)];

        let dep_info_1 = vk::DependencyInfo::default().image_memory_barriers(&barrier_1);

        let barrier_2 = [vk::ImageMemoryBarrier2::default()
            .src_stage_mask(vk::PipelineStageFlags2::TRANSFER)
            .src_access_mask(vk::AccessFlags2::TRANSFER_WRITE)
            .dst_stage_mask(vk::PipelineStageFlags2::FRAGMENT_SHADER)
            .dst_access_mask(vk::AccessFlags2::SHADER_READ)
            .old_layout(vk::ImageLayout::TRANSFER_DST_OPTIMAL)
            .new_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
            .image(self.image)
            .subresource_range(self.image_srr)];

        let dep_info_2 = vk::DependencyInfo::default().image_memory_barriers(&barrier_2);

        let cmd_buffer = self.context.borrow().immediate_command_buffer;

        let sr_layers = vk::ImageSubresourceLayers::default()
            .aspect_mask(self.image_srr.aspect_mask)
            .mip_level(self.image_srr.base_mip_level)
            .base_array_layer(self.image_srr.base_array_layer)
            .layer_count(self.image_srr.layer_count);

        let copy_region = [vk::BufferImageCopy::default()
            .buffer_offset(0)
            .buffer_row_length(0)
            .buffer_image_height(0)
            .image_subresource(sr_layers)
            .image_extent(self.image_extent)];

        unsafe {
            self.context
                .borrow()
                .device
                .cmd_pipeline_barrier2(cmd_buffer, &dep_info_1);

            self.context.borrow().device.cmd_copy_buffer_to_image(
                cmd_buffer,
                src_buffer.get_buffer(),
                self.image,
                vk::ImageLayout::TRANSFER_DST_OPTIMAL,
                &copy_region,
            );

            self.context
                .borrow()
                .device
                .cmd_pipeline_barrier2(cmd_buffer, &dep_info_2);
        }

        self.context.borrow().cmd_immediate_end()?;

        self.context.borrow().cmd_immediate_submit()
    }
}

impl Drop for Texture {
    fn drop(&mut self) {
        unsafe {
            self.context
                .borrow()
                .device
                .destroy_image_view(self.image_view, None);

            self.context
                .borrow()
                .memory_allocator
                .destroy_image(self.image, &mut self.image_memory);
        }
    }
}
