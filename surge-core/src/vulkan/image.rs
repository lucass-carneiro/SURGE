use super::{AllocatedImage, DPETH_FORMAT, SwapchainData};
use crate::errors::VulkanError;
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

    let surface_formats = unsafe {
        surface_loader
            .get_physical_device_surface_formats(physical_device, surface)
            .map_err(|e| VulkanError::SurfaceFormatQueryError(e))?
    };

    // Device selection guarantees at least one format. The renderer's colour pipeline is linear:
    // textures are sampled through `_SRGB` views (decoded to linear), blending happens in linear
    // space, and an `_SRGB` swapchain encodes on write. So an sRGB format is required for correct
    // colours; UNORM is only a degraded fallback.
    let surface_format = [vk::Format::B8G8R8A8_SRGB, vk::Format::R8G8B8A8_SRGB]
        .into_iter()
        .find_map(|wanted| {
            surface_formats.iter().copied().find(|f| {
                f.format == wanted && f.color_space == vk::ColorSpaceKHR::SRGB_NONLINEAR
            })
        })
        .unwrap_or_else(|| {
            log::warn!(
                "Surface offers no sRGB format, falling back to {:?}/{:?}; colours will be too dark",
                surface_formats[0].format,
                surface_formats[0].color_space
            );
            surface_formats[0]
        });
    let format = surface_format.format;
    let color_space = surface_format.color_space;

    let image_count = if surface_capabilities.max_image_count > 0 {
        surface_capabilities
            .max_image_count
            .min(surface_capabilities.min_image_count + 1)
    } else {
        surface_capabilities.min_image_count + 1
    };

    let present_modes = unsafe {
        surface_loader
            .get_physical_device_surface_present_modes(physical_device, surface)
            .map_err(|e| VulkanError::SurfacePresentModeQueryError(e))?
    };

    // FIFO is the only mode the spec guarantees.
    let present_mode = if !vsync && present_modes.contains(&vk::PresentModeKHR::IMMEDIATE) {
        vk::PresentModeKHR::IMMEDIATE
    } else {
        if !vsync {
            log::warn!("IMMEDIATE present mode unavailable, falling back to FIFO");
        }
        vk::PresentModeKHR::FIFO
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
    swapchain_extent: vk::Extent2D,
) -> Result<AllocatedImage, VulkanError> {
    // Must match the swapchain (and so the render area), not the configured design resolution:
    // in borderless fullscreen the surface is the monitor size.
    let extent = vk::Extent3D {
        width: swapchain_extent.width,
        height: swapchain_extent.height,
        depth: 1,
    };

    let image_ci = vk::ImageCreateInfo {
        image_type: vk::ImageType::TYPE_2D,
        format: DPETH_FORMAT,
        extent: extent,
        mip_levels: 1,
        array_layers: 1,
        samples: vk::SampleCountFlags::TYPE_1,
        tiling: vk::ImageTiling::OPTIMAL,
        usage: vk::ImageUsageFlags::DEPTH_STENCIL_ATTACHMENT,
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
        format: DPETH_FORMAT,
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

/// Whether writing to an attachment of `format` applies the sRGB transfer function.
pub(super) fn is_srgb_format(format: vk::Format) -> bool {
    matches!(
        format,
        vk::Format::B8G8R8A8_SRGB | vk::Format::R8G8B8A8_SRGB
    )
}

/// The sRGB electro-optical transfer function: an encoded value in `[0, 1]` to linear light.
pub(super) fn srgb_to_linear(c: f32) -> f32 {
    if c <= 0.04045 {
        c / 12.92
    } else {
        ((c + 0.055) / 1.055).powf(2.4)
    }
}

#[cfg(test)]
mod tests {
    use super::srgb_to_linear;

    #[test]
    fn srgb_to_linear_matches_reference_points() {
        assert_eq!(srgb_to_linear(0.0), 0.0);
        assert!((srgb_to_linear(1.0) - 1.0).abs() < 1e-6);
        // Mid-grey 0.5 encoded is ~0.2140 linear.
        assert!((srgb_to_linear(0.5) - 0.2140).abs() < 1e-3);
        // Linear segment.
        assert!((srgb_to_linear(0.04) - 0.04 / 12.92).abs() < 1e-7);
    }
}
