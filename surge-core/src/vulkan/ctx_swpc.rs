use super::{FRAMES_IN_FLIGHT, SwapchainImageData, VulkanContext, image};
use crate::{config::EngineConfig, errors::VulkanError};
use ash::vk;

impl VulkanContext {
    pub fn request_swpc_img(
        &mut self,
        config: &EngineConfig,
    ) -> Result<SwapchainImageData, VulkanError> {
        let timeout = 1000000000;

        // Wait frame fences
        unsafe {
            self.device
                .wait_for_fences(&[self.frame_fences[self.current_frame]], true, timeout)
                .map_err(|e| VulkanError::FenceWaiteError(e))?;
        }

        // Acquire next image
        let (index, suboptimal) = unsafe {
            self.swapchain_data
                .swapchain_loader
                .acquire_next_image(
                    self.swapchain_data.swapchain,
                    timeout,
                    self.present_completed_sem[self.semaphore_index],
                    vk::Fence::null(),
                )
                .map_err(|e| VulkanError::SwapchainAcquireError(e))
        }?;

        let swpc_img_data = SwapchainImageData { index, suboptimal };

        // Only reset the fence if we are submitting work
        if swpc_img_data.suboptimal {
            self.recreate_swapchain(&config)?;
            return Ok(swpc_img_data);
        }

        unsafe {
            self.device
                .reset_fences(&[self.frame_fences[self.current_frame]])
                .map_err(|e| VulkanError::FenceResetError(e))
        }?;

        Ok(swpc_img_data)
    }

    pub fn present_swpc(
        &mut self,
        swpc_img_data: &mut SwapchainImageData,
        config: &EngineConfig,
    ) -> Result<(), VulkanError> {
        let pi = vk::PresentInfoKHR {
            wait_semaphore_count: 1,
            p_wait_semaphores: &self.render_finished_sem[self.semaphore_index],
            swapchain_count: 1,
            p_swapchains: &self.swapchain_data.swapchain,
            p_image_indices: &swpc_img_data.index,
            ..Default::default()
        };

        swpc_img_data.suboptimal = unsafe {
            self.swapchain_data
                .swapchain_loader
                .queue_present(self.graphics_queue, &pi)
                .map_err(|e| VulkanError::SwapchainPresentError(self.current_frame, e))
        }?;

        if swpc_img_data.suboptimal {
            self.recreate_swapchain(config)?;
        }

        self.semaphore_index = (self.semaphore_index + 1) % self.present_completed_sem.len();
        self.current_frame = (self.current_frame + 1) % FRAMES_IN_FLIGHT;

        Ok(())
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

        self.depth_image = image::create_depth_image(&self.memory_allocator, &self.device, config)?;

        Ok(())
    }
}
