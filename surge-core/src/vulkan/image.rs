use super::{AllocatedImage, SwapchainData};
use crate::{config::EngineConfig, errors::VulkanError};
use ash::{self, khr, vk};
use vk_mem::Alloc;

pub(super) fn create_swapchain(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
    device: &ash::Device,
    surface_loader: &khr::surface::Instance,
    surface: vk::SurfaceKHR,
    width: u32,
    height: u32,
    vsync: bool,
) -> Result<SwapchainData, VulkanError> {
    log::info!("Creating swapchain");

    let surface_capabilities = unsafe {
        surface_loader
            .get_physical_device_surface_capabilities(physical_device, surface)
            .map_err(|e| VulkanError::SurfaceCapabilityQueryError(e))?
    };

    let extent = vk::Extent2D {
        width: width.clamp(
            surface_capabilities.min_image_extent.width,
            surface_capabilities.max_image_extent.width,
        ),
        height: height.clamp(
            surface_capabilities.min_image_extent.height,
            surface_capabilities.max_image_extent.height,
        ),
    };

    let format = vk::Format::B8G8R8A8_UNORM;
    let color_space = vk::ColorSpaceKHR::SRGB_NONLINEAR;

    let image_count = if surface_capabilities.max_image_count > 0 {
        surface_capabilities
            .max_image_count
            .min(surface_capabilities.min_image_count + 1)
    } else {
        surface_capabilities.min_image_count + 1
    };

    let present_mode = if vsync {
        vk::PresentModeKHR::FIFO
    } else {
        vk::PresentModeKHR::IMMEDIATE
    };

    let swapchain_create_info = vk::SwapchainCreateInfoKHR::default()
        .surface(surface)
        .min_image_count(image_count)
        .image_format(format)
        .image_color_space(color_space)
        .image_extent(extent)
        .image_array_layers(1)
        .image_usage(vk::ImageUsageFlags::TRANSFER_DST | vk::ImageUsageFlags::COLOR_ATTACHMENT)
        .image_sharing_mode(vk::SharingMode::EXCLUSIVE)
        .pre_transform(surface_capabilities.current_transform)
        .composite_alpha(vk::CompositeAlphaFlagsKHR::OPAQUE)
        .present_mode(present_mode)
        .clipped(true);

    let swapchain_loader = khr::swapchain::Device::new(instance, device);
    let swapchain = unsafe {
        swapchain_loader
            .create_swapchain(&swapchain_create_info, None)
            .map_err(|e| VulkanError::SwapchainCreationError(e))?
    };

    let images = unsafe {
        swapchain_loader
            .get_swapchain_images(swapchain)
            .map_err(|e| VulkanError::SwapchainCreationError(e))?
    };

    let mut image_views = Vec::new();

    for image in &images {
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
            format: format,
            subresource_range: sr,
            image: *image,
            ..Default::default()
        };

        image_views.push(unsafe {
            device
                .create_image_view(&ivci, None)
                .map_err(|e| VulkanError::SwapchainCreationError(e))
        }?);
    }

    Ok(SwapchainData {
        swapchain_loader,
        swapchain,
        images,
        image_views,
        format,
        extent,
    })
}

pub(super) fn create_depth_image(
    memory_allocator: &vk_mem::Allocator,
    device: &ash::Device,
    config: &EngineConfig,
) -> Result<AllocatedImage, VulkanError> {
    let extent = vk::Extent3D {
        width: config.resolution.width,
        height: config.resolution.height,
        depth: 1,
    };

    let format = vk::Format::D32_SFLOAT;

    let usage_flags = vk::ImageUsageFlags::DEPTH_STENCIL_ATTACHMENT;

    let image_ci = vk::ImageCreateInfo {
        image_type: vk::ImageType::TYPE_2D,
        format: format,
        extent: extent,
        mip_levels: 1,
        array_layers: 1,
        samples: vk::SampleCountFlags::TYPE_1,
        tiling: vk::ImageTiling::OPTIMAL,
        usage: usage_flags,
        ..Default::default()
    };

    let alloc_ci = vk_mem::AllocationCreateInfo {
        usage: vk_mem::MemoryUsage::AutoPreferDevice,
        ..Default::default()
    };

    let (depth_image, depth_image_allocation) = unsafe {
        memory_allocator
            .create_image(&image_ci, &alloc_ci)
            .map_err(|e| VulkanError::DepthImageCreationError(e))
    }?;

    let sr = vk::ImageSubresourceRange {
        aspect_mask: vk::ImageAspectFlags::DEPTH,
        base_mip_level: 0,
        level_count: 1,
        base_array_layer: 0,
        layer_count: 1,
        ..Default::default()
    };

    let ivci = vk::ImageViewCreateInfo {
        view_type: vk::ImageViewType::TYPE_2D,
        format: format,
        subresource_range: sr,
        image: depth_image,
        ..Default::default()
    };

    let depth_image_view = unsafe {
        device
            .create_image_view(&ivci, None)
            .map_err(|e| VulkanError::SwapchainCreationError(e))
    }?;

    Ok(AllocatedImage {
        image: depth_image,
        image_view: depth_image_view,
        image_memory: depth_image_allocation,
    })
}
