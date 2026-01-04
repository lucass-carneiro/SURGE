use super::{VulkanContext, command, device, image, instance};
use crate::{config::EngineConfig, errors::VulkanError};
use ash::{self, ext, khr, vk};
use std::mem::ManuallyDrop;
use winit::{
    event_loop::ActiveEventLoop,
    raw_window_handle::{HasDisplayHandle, HasWindowHandle},
    window::Window,
};

impl VulkanContext {
    pub fn new(
        event_loop: &ActiveEventLoop,
        window: &Window,
        config: &EngineConfig,
    ) -> Result<Self, VulkanError> {
        log::info!("Initializing Vulkan");

        let entry = unsafe { ash::Entry::load().map_err(|e| VulkanError::LibraryLoadingError(e))? };

        let api_version = unsafe {
            entry
                .try_enumerate_instance_version()
                .map_err(|e| VulkanError::InstanceCreationError(e))?
                .unwrap_or(vk::API_VERSION_1_0)
        };

        log::info!(
            "Vulkan supported API version: {}.{}.{}",
            vk::api_version_major(api_version),
            vk::api_version_minor(api_version),
            vk::api_version_patch(api_version)
        );

        let extensions = instance::get_required_instance_extensions(event_loop)?;

        #[cfg(feature = "validation_layers")]
        let layers = instance::get_required_validation_layers(&entry)?;

        #[cfg(feature = "validation_layers")]
        let instance = instance::build_instance(&entry, &extensions, &layers)?;

        #[cfg(not(feature = "validation_layers"))]
        let instance = build_instance(&entry, &extensions)?;

        #[cfg(feature = "validation_layers")]
        let (debug_utils, debug_messenger) = {
            let debug_utils = ext::debug_utils::Instance::new(&entry, &instance);
            let debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT::default()
                .message_severity(
                    vk::DebugUtilsMessageSeverityFlagsEXT::INFO
                        | vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE
                        | vk::DebugUtilsMessageSeverityFlagsEXT::WARNING
                        | vk::DebugUtilsMessageSeverityFlagsEXT::ERROR,
                )
                .message_type(
                    vk::DebugUtilsMessageTypeFlagsEXT::GENERAL
                        | vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION
                        | vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE,
                )
                .pfn_user_callback(Some(instance::debug_callback));

            let messenger = unsafe {
                debug_utils
                    .create_debug_utils_messenger(&debug_create_info, None)
                    .map_err(|e| VulkanError::DebugMessengerCreationError(e))?
            };

            (debug_utils, messenger)
        };

        let physical_device = device::select_physical_device(&instance)?;

        let (device, graphics_queue, compute_queue, transfer_queue) =
            device::create_logical_device(&instance, physical_device)?;

        let surface_loader = khr::surface::Instance::new(&entry, &instance);
        let surface_khr = unsafe {
            ash_window::create_surface(
                &entry,
                &instance,
                window.display_handle().unwrap().as_raw(),
                window.window_handle().unwrap().as_raw(),
                None,
            )
            .map_err(|e| VulkanError::SurfaceCreationError(e))?
        };

        let swapchain_data = image::create_swapchain(
            &instance,
            physical_device,
            &device,
            &surface_loader,
            surface_khr,
            config.resolution.width,
            config.resolution.height,
            config.renderer.vsync,
        )?;

        let queue_families =
            unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
        let indices = device::get_queue_family_indices(&queue_families);

        let command_pool = command::create_command_pool(&device, indices.graphics)?;
        let command_buffers = command::create_command_buffers(&device, command_pool)?;

        let immediate_command_buffer = {
            let allocate_info = vk::CommandBufferAllocateInfo::default()
                .command_pool(command_pool)
                .level(vk::CommandBufferLevel::PRIMARY)
                .command_buffer_count(1);

            unsafe {
                device
                    .allocate_command_buffers(&allocate_info)
                    .map_err(|e| VulkanError::CommandBufferAllocation(e))
            }
        }?[0];

        let memory_allocator = ManuallyDrop::new(command::create_memory_allocator(
            &instance,
            &device,
            physical_device,
        )?);

        let depth_image = image::create_depth_image(&memory_allocator, &device, config)?;

        let present_completed_sem =
            command::create_semaphores(&device, swapchain_data.images.len())?;
        let render_finished_sem = command::create_semaphores(&device, swapchain_data.images.len())?;
        let frame_fences = command::create_fences(&device)?;

        log::info!("Vulkan context created");

        Ok(Self {
            entry,
            instance,
            #[cfg(feature = "validation_layers")]
            debug_utils,
            #[cfg(feature = "validation_layers")]
            debug_messenger,
            physical_device,
            device,
            indices,
            graphics_queue,
            compute_queue,
            transfer_queue,
            surface: surface_loader,
            surface_khr,
            swapchain_data,
            depth_image,
            command_pool,
            command_buffers,
            immediate_command_buffer,
            memory_allocator,
            present_completed_sem,
            render_finished_sem,
            frame_fences,
            current_frame: 0,
            semaphore_index: 0,
        })
    }

    /// Manually destroy the swapchain
    ///
    /// **IMPORTANT**: This MUST be called before the window is destroyed/closed
    /// to avoid segfaults. The swapchain depends on the window surface, and if
    /// the surface is destroyed first (by winit), attempting to destroy the
    /// swapchain will cause a crash.
    ///
    /// Call this in your application's cleanup code before dropping the window,
    /// or in response to window close events.
    pub fn destroy_swapchain(&mut self) -> Result<(), VulkanError> {
        unsafe {
            if self.swapchain_data.swapchain != vk::SwapchainKHR::null() {
                self.device
                    .device_wait_idle()
                    .map_err(|e| VulkanError::SwapchainDestructionError(e))?;

                for image_view in &self.swapchain_data.image_views {
                    self.device.destroy_image_view(*image_view, None);
                }

                self.swapchain_data
                    .swapchain_loader
                    .destroy_swapchain(self.swapchain_data.swapchain, None);
            }
        }

        Ok(())
    }

    pub(crate) fn destroy_depth_image(&mut self) {
        unsafe {
            self.device
                .destroy_image_view(self.depth_image.image_view, None);
            self.memory_allocator
                .destroy_image(self.depth_image.image, &mut self.depth_image.image_memory)
        };
    }
}

impl Drop for VulkanContext {
    fn drop(&mut self) {
        log::info!("Cleaning up Vulkan resources");

        unsafe {
            // Wait for device to be idle before destroying anything
            if let Err(e) = self.device.device_wait_idle() {
                log::error!("Failed to wait for device idle during cleanup: {:?}", e);
                return; // Don't proceed with cleanup if device is in bad state
            }

            log::debug!("Destroying sync objects");
            // Destroy sync objects
            for fence in &self.frame_fences {
                self.device.destroy_fence(*fence, None);
            }
            for semaphore in &self.present_completed_sem {
                self.device.destroy_semaphore(*semaphore, None);
            }
            for semaphore in &self.render_finished_sem {
                self.device.destroy_semaphore(*semaphore, None);
            }

            // This also frees command buffers
            log::debug!("Destroying command pool");
            self.device.destroy_command_pool(self.command_pool, None);

            log::debug!("Skipping swapchain destruction in Drop");
            // NOTE: Swapchain destruction MUST happen before the window/surface is destroyed.
            // Since winit may destroy the surface before our Drop runs, we cannot safely
            // destroy the swapchain here. The swapchain should be destroyed manually by
            // calling destroy_swapchain() before the window is closed.
            //
            // If not destroyed manually, the device destruction below will implicitly
            // clean up the swapchain, but this will trigger validation layer warnings.
            // This is a known limitation when integrating with window libraries.

            log::debug!("Destroying depth buffer");
            self.destroy_depth_image();

            log::debug!("Dropping memory allocator");
            // Explicitly drop the memory allocator before destroying the device
            // This ensures all VkDeviceMemory allocations are freed
            // ManuallyDrop::drop ensures this is only dropped once
            ManuallyDrop::drop(&mut self.memory_allocator);

            log::debug!("About to destroy device");
            // Destroy device - this must happen before destroying the instance
            self.device.destroy_device(None);

            log::debug!("Destroying surface");
            // Destroy surface after device but before instance
            // Surface is owned by instance, not device
            if self.surface_khr != vk::SurfaceKHR::null() {
                self.surface.destroy_surface(self.surface_khr, None);
            }

            log::debug!("Destroying debug messenger");
            // Destroy debug messenger before instance
            #[cfg(feature = "validation_layers")]
            self.debug_utils
                .destroy_debug_utils_messenger(self.debug_messenger, None);

            log::debug!("Destroying instance");
            // Destroy instance last
            self.instance.destroy_instance(None);
        }

        log::info!("Vulkan cleanup complete");
    }
}
