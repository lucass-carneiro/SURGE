use super::{SwpcLayoutTransitionInfo, VulkanContext};
use crate::{config::EngineConfig, errors::VulkanError};
use ash::vk;

impl VulkanContext {
    fn cmd_transition_swpc_image_layout(&self, ti: SwpcLayoutTransitionInfo) {
        let subresource_range = vk::ImageSubresourceRange {
            aspect_mask: vk::ImageAspectFlags::COLOR,
            base_mip_level: 0,
            level_count: 1,
            base_array_layer: 0,
            layer_count: 1,
            ..Default::default()
        };

        let barrier = vk::ImageMemoryBarrier2 {
            src_stage_mask: ti.src_stage_mask,
            src_access_mask: ti.src_access_mask,
            dst_stage_mask: ti.dst_stage_mask,
            dst_access_mask: ti.dst_access_mask,
            old_layout: ti.old_layout,
            new_layout: ti.new_layout,
            image: self.swapchain_data.images[ti.image_index as usize],
            subresource_range: subresource_range,
            ..Default::default()
        };

        let dependency_info = vk::DependencyInfo {
            image_memory_barrier_count: 1,
            p_image_memory_barriers: &barrier,
            ..Default::default()
        };

        unsafe {
            self.device
                .cmd_pipeline_barrier2(self.command_buffers[self.current_frame], &dependency_info)
        }
    }

    pub fn cmd_begin(&self, swpc_img_idx: u32) -> Result<(), VulkanError> {
        // Reset command buffer
        unsafe {
            self.device
                .reset_command_buffer(
                    self.command_buffers[self.current_frame],
                    vk::CommandBufferResetFlags::empty(),
                )
                .map_err(|e| VulkanError::FrameCommandBufferReset(self.current_frame, e))
        }?;

        // Begin command recording
        unsafe {
            let bi = vk::CommandBufferBeginInfo {
                flags: vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT,
                ..Default::default()
            };

            self.device
                .begin_command_buffer(self.command_buffers[self.current_frame], &bi)
                .map_err(|e| VulkanError::FrameCommandBufferRecordBeginError(self.current_frame, e))
        }?;

        // Transition swapchain image
        let swpc_ti = SwpcLayoutTransitionInfo {
            image_index: swpc_img_idx,
            old_layout: vk::ImageLayout::UNDEFINED,
            new_layout: vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            src_access_mask: vk::AccessFlags2::empty(),
            dst_access_mask: vk::AccessFlags2::COLOR_ATTACHMENT_WRITE,
            src_stage_mask: vk::PipelineStageFlags2::COLOR_ATTACHMENT_OUTPUT,
            dst_stage_mask: vk::PipelineStageFlags2::COLOR_ATTACHMENT_OUTPUT,
        };

        self.cmd_transition_swpc_image_layout(swpc_ti);

        Ok(())
    }

    pub fn cmd_render_begin(&self, swpc_img_idx: u32, config: &EngineConfig) {
        let ccl = vk::ClearColorValue {
            float32: [
                config.clear_color.r as f32,
                config.clear_color.g as f32,
                config.clear_color.b as f32,
                config.clear_color.a as f32,
            ],
        };

        let cdp = vk::ClearDepthStencilValue {
            depth: 0.0,
            stencil: 0,
        };

        let cai = vk::RenderingAttachmentInfo {
            image_view: self.swapchain_data.image_views[swpc_img_idx as usize],
            image_layout: vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            load_op: vk::AttachmentLoadOp::CLEAR,
            store_op: vk::AttachmentStoreOp::STORE,
            clear_value: vk::ClearValue { color: ccl },
            ..Default::default()
        };

        let dai = vk::RenderingAttachmentInfo {
            image_view: self.depth_image.image_view,
            image_layout: vk::ImageLayout::DEPTH_ATTACHMENT_OPTIMAL,
            load_op: vk::AttachmentLoadOp::CLEAR,
            store_op: vk::AttachmentStoreOp::STORE,
            clear_value: vk::ClearValue { depth_stencil: cdp },
            ..Default::default()
        };

        let ra = vk::Rect2D {
            offset: vk::Offset2D { x: 0, y: 0 },
            extent: self.swapchain_data.extent,
        };

        let ri = vk::RenderingInfo {
            render_area: ra,
            layer_count: 1,
            color_attachment_count: 1,
            p_color_attachments: &cai,
            p_depth_attachment: &dai,
            ..Default::default()
        };

        unsafe {
            self.device
                .cmd_begin_rendering(self.command_buffers[self.current_frame], &ri)
        }
    }

    pub fn cmd_render_end(&self) {
        unsafe {
            self.device
                .cmd_end_rendering(self.command_buffers[self.current_frame]);
        }
    }

    pub fn cmd_end(&self, swpc_img_idx: u32) -> Result<(), VulkanError> {
        // Transition swapchain image
        let swpc_ti = SwpcLayoutTransitionInfo {
            image_index: swpc_img_idx,
            old_layout: vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            new_layout: vk::ImageLayout::PRESENT_SRC_KHR,
            src_access_mask: vk::AccessFlags2::COLOR_ATTACHMENT_WRITE,
            dst_access_mask: vk::AccessFlags2::empty(),
            src_stage_mask: vk::PipelineStageFlags2::COLOR_ATTACHMENT_OUTPUT,
            dst_stage_mask: vk::PipelineStageFlags2::BOTTOM_OF_PIPE,
        };
        self.cmd_transition_swpc_image_layout(swpc_ti);

        // End recording
        unsafe {
            self.device
                .end_command_buffer(self.command_buffers[self.current_frame])
                .map_err(|e| VulkanError::FrameCommandBufferRecordEndError(self.current_frame, e))
        }
    }

    pub fn cmd_submit(&self) -> Result<(), VulkanError> {
        let si = vk::SubmitInfo {
            wait_semaphore_count: 1,
            p_wait_semaphores: &self.present_completed_sem[self.semaphore_index],
            p_wait_dst_stage_mask: &vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,
            command_buffer_count: 1,
            p_command_buffers: &self.command_buffers[self.current_frame],
            signal_semaphore_count: 1,
            p_signal_semaphores: &self.render_finished_sem[self.semaphore_index],
            ..Default::default()
        };

        unsafe {
            self.device
                .queue_submit(
                    self.graphics_queue,
                    &[si],
                    self.frame_fences[self.current_frame],
                )
                .map_err(|e| VulkanError::FrameCommandBufferSubmitError(self.current_frame, e))
        }
    }
}
