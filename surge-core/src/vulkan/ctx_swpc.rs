use super::{FRAMES_IN_FLIGHT, SwapchainImageData, VulkanContext, command, image};
use crate::{config::EngineConfig, errors::VulkanError};
use ash::vk;

impl VulkanContext {
    /// Returns `Ok(None)` when the swapchain is out of date: there is no valid image to render
    /// into, so the caller must skip the frame entirely and recreate the swapchain instead.
    pub fn request_swpc_img(&mut self) -> Result<Option<SwapchainImageData>, VulkanError> {
        let timeout = 1000000000;

        // Wait frame fences
        unsafe {
            self.device
                .wait_for_fences(&[self.frame_fences[self.current_frame]], true, timeout)
                .map_err(|e| VulkanError::FenceWaiteError(e))?;
        }

        // Acquire next image
        let acquire_result = unsafe {
            self.swapchain_data.swapchain_loader.acquire_next_image(
                self.swapchain_data.swapchain,
                timeout,
                self.present_completed_sem[self.current_frame],
                vk::Fence::null(),
            )
        };

        let (index, suboptimal) = match acquire_result {
            Ok(v) => v,
            Err(vk::Result::ERROR_OUT_OF_DATE_KHR) => return Ok(None),
            Err(e) => return Err(VulkanError::SwapchainAcquireError(e)),
        };

        // Store the image index for use by cmd_submit (render_finished_sem is indexed by image)
        self.current_image_index = index as usize;

        let swpc_img_data = SwapchainImageData { index, suboptimal };

        unsafe {
            self.device
                .reset_fences(&[self.frame_fences[self.current_frame]])
                .map_err(|e| VulkanError::FenceResetError(e))
        }?;

        Ok(Some(swpc_img_data))
    }

    /// Returns `Ok(true)` when the swapchain is out of date and must be recreated before the
    /// next frame; the current frame's GPU work has already been submitted either way.
    pub fn present_swpc(
        &mut self,
        swpc_img_data: &mut SwapchainImageData,
    ) -> Result<bool, VulkanError> {
        // render_finished_sem is indexed by image index because the presentation engine
        // holds the semaphore until this specific image is re-acquired
        let pi = vk::PresentInfoKHR {
            wait_semaphore_count: 1,
            p_wait_semaphores: &self.render_finished_sem[self.current_image_index],
            swapchain_count: 1,
            p_swapchains: &self.swapchain_data.swapchain,
            p_image_indices: &swpc_img_data.index,
            ..Default::default()
        };

        let present_result = unsafe {
            self.swapchain_data
                .swapchain_loader
                .queue_present(self.graphics_queue, &pi)
        };

        let out_of_date = match present_result {
            Ok(suboptimal) => {
                swpc_img_data.suboptimal = suboptimal;
                false
            }
            Err(vk::Result::ERROR_OUT_OF_DATE_KHR) => true,
            Err(e) => return Err(VulkanError::SwapchainPresentError(self.current_frame, e)),
        };

        self.current_frame = (self.current_frame + 1) % FRAMES_IN_FLIGHT;

        Ok(out_of_date)
    }

    pub fn recreate_swapchain(&mut self, config: &EngineConfig) -> Result<(), VulkanError> {
        log::info!("Recreating swapchain");

        unsafe {
            self.device
                .device_wait_idle()
                .map_err(|e| VulkanError::SwapchainRecreationError(e))
        }?;

        self.destroy_swapchain()?;
        self.destroy_depth_image();

        // Destroy old synchronization objects - they may have stale state from the old swapchain
        unsafe {
            for semaphore in &self.present_completed_sem {
                self.device.destroy_semaphore(*semaphore, None);
            }
            for semaphore in &self.render_finished_sem {
                self.device.destroy_semaphore(*semaphore, None);
            }
            for fence in &self.frame_fences {
                self.device.destroy_fence(*fence, None);
            }
        }

        // Create swapchain first to know the image count for semaphores
        self.swapchain_data = image::create_swapchain(
            &self.instance,
            self.physical_device,
            &self.device,
            &self.surface,
            self.surface_khr,
            config.resolution.width,
            config.resolution.height,
            config.renderer.vsync,
        )?;

        // Create fresh synchronization objects sized to swapchain image count
        let swapchain_image_count = self.swapchain_data.images.len();
        self.present_completed_sem = command::create_semaphores(&self.device, swapchain_image_count)?;
        self.render_finished_sem = command::create_semaphores(&self.device, swapchain_image_count)?;
        self.frame_fences = command::create_fences(&self.device)?;

        // Reset frame counters to ensure clean synchronization state
        self.current_frame = 0;
        self.current_image_index = 0;

        self.depth_image = image::create_depth_image(&self.memory_allocator, &self.device, config)?;

        Ok(())
    }
}
