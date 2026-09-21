use super::QueueFamilyIndices;
use crate::errors::VulkanError;
use ash::{khr, vk};
use std::ffi::{CStr, c_void};

fn get_available_physical_devices(
    instance: &ash::Instance,
) -> Result<Vec<vk::PhysicalDevice>, VulkanError> {
    unsafe {
        instance
            .enumerate_physical_devices()
            .map_err(|e| VulkanError::PhysicalDeviceListError(e))
    }
}

// TAG: Device features
fn device_has_required_features(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> bool {
    let mut features13 = vk::PhysicalDeviceVulkan13Features::default();
    let mut features12 = vk::PhysicalDeviceVulkan12Features::default();
    let mut features11 = vk::PhysicalDeviceVulkan11Features::default();

    features11.p_next = &mut features12 as *mut _ as *mut c_void;
    features12.p_next = &mut features13 as *mut _ as *mut c_void;

    let mut features2 = vk::PhysicalDeviceFeatures2::default();
    features2.p_next = &mut features11 as *mut _ as *mut c_void;

    unsafe {
        instance.get_physical_device_features2(physical_device, &mut features2);
    }

    features2.features.sampler_anisotropy == vk::TRUE
        && features2.features.shader_int64 == vk::TRUE
        && features12.buffer_device_address == vk::TRUE
        && features12.descriptor_indexing == vk::TRUE
        && features12.shader_sampled_image_array_non_uniform_indexing == vk::TRUE
        && features12.runtime_descriptor_array == vk::TRUE
        && features12.descriptor_binding_variable_descriptor_count == vk::TRUE
        && features12.descriptor_binding_partially_bound == vk::TRUE
        && features13.dynamic_rendering == vk::TRUE
        && features13.synchronization2 == vk::TRUE
}

// TAG: Device extensions
fn get_required_device_extensions() -> Vec<*const i8> {
    vec![khr::swapchain::NAME.as_ptr()]
}

fn device_has_required_extensions(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> bool {
    let available_extensions = unsafe {
        match instance.enumerate_device_extension_properties(physical_device) {
            Ok(ext) => ext,
            Err(_) => return false,
        }
    };

    let required = get_required_device_extensions();

    for req_ext in required {
        let req_name = unsafe { CStr::from_ptr(req_ext) };
        let mut found = false;

        for av_ext in &available_extensions {
            let av_name = unsafe { CStr::from_ptr(av_ext.extension_name.as_ptr()) };
            if req_name == av_name {
                found = true;
                break;
            }
        }

        if !found {
            return false;
        }
    }

    true
}

fn device_has_required_queue_families(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> bool {
    let queue_families =
        unsafe { instance.get_physical_device_queue_family_properties(physical_device) };

    let mut has_graphics = false;
    let mut has_compute = false;
    let mut has_transfer = false;

    for family in queue_families {
        if family.queue_flags.contains(vk::QueueFlags::GRAPHICS) {
            has_graphics = true;
        }
        if family.queue_flags.contains(vk::QueueFlags::COMPUTE) {
            has_compute = true;
        }
        if family.queue_flags.contains(vk::QueueFlags::TRANSFER) {
            has_transfer = true;
        }
    }

    has_graphics && has_compute && has_transfer
}

/// First graphics-capable queue family that can also present to `surface`. Frames are presented
/// on the graphics queue, so this is the family `QueueFamilyIndices::graphics` must be.
fn find_graphics_present_family(
    queue_families: &[vk::QueueFamilyProperties],
    physical_device: vk::PhysicalDevice,
    surface_loader: &khr::surface::Instance,
    surface: vk::SurfaceKHR,
) -> Option<u32> {
    queue_families
        .iter()
        .enumerate()
        .filter(|(_, family)| family.queue_flags.contains(vk::QueueFlags::GRAPHICS))
        .map(|(i, _)| i as u32)
        .find(|&i| unsafe {
            surface_loader
                .get_physical_device_surface_support(physical_device, i, surface)
                .unwrap_or(false)
        })
}

fn device_has_surface_formats_and_present_modes(
    physical_device: vk::PhysicalDevice,
    surface_loader: &khr::surface::Instance,
    surface: vk::SurfaceKHR,
) -> bool {
    unsafe {
        let formats = surface_loader.get_physical_device_surface_formats(physical_device, surface);
        let modes =
            surface_loader.get_physical_device_surface_present_modes(physical_device, surface);
        matches!(formats, Ok(f) if !f.is_empty()) && matches!(modes, Ok(m) if !m.is_empty())
    }
}

/// `graphics` is chosen by the caller (it depends on the surface); compute/transfer are not
/// presentation-related.
pub(super) fn get_queue_family_indices(
    queue_families: &[vk::QueueFamilyProperties],
    graphics: u32,
) -> QueueFamilyIndices {
    let mut indices = QueueFamilyIndices {
        graphics,
        compute: 0,
        transfer: 0,
    };

    for (i, family) in queue_families.iter().enumerate() {
        if family.queue_flags.contains(vk::QueueFlags::COMPUTE) {
            indices.compute = i as u32;
        }
        if family.queue_flags.contains(vk::QueueFlags::TRANSFER) {
            indices.transfer = i as u32;
        }
    }

    indices
}

fn is_device_suitable(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
    surface_loader: &khr::surface::Instance,
    surface: vk::SurfaceKHR,
) -> Option<QueueFamilyIndices> {
    let properties = unsafe { instance.get_physical_device_properties(physical_device) };
    let device_name = unsafe {
        CStr::from_ptr(properties.device_name.as_ptr())
            .to_string_lossy()
            .to_string()
    };

    log::info!("Checking device suitability of {}", device_name);

    let has_required_features = device_has_required_features(instance, physical_device);
    let has_device_extensions = device_has_required_extensions(instance, physical_device);
    let has_queue_families = device_has_required_queue_families(instance, physical_device);

    let queue_families =
        unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
    let graphics_family = find_graphics_present_family(
        &queue_families,
        physical_device,
        surface_loader,
        surface,
    );
    let can_present = graphics_family.is_some()
        && device_has_surface_formats_and_present_modes(physical_device, surface_loader, surface);

    let is_suitable =
        has_required_features && has_device_extensions && has_queue_families && can_present;

    if let (true, Some(graphics)) = (is_suitable, graphics_family) {
        log::info!("Device is suitable");
        Some(get_queue_family_indices(&queue_families, graphics))
    } else {
        log::info!(
            "Device is unsuitable:
  Has required features? {}
  Has required device extensions? {}
  Has required queue families? {}
  Can present to the window surface? {}",
            has_required_features,
            has_device_extensions,
            has_queue_families,
            can_present
        );
        None
    }
}

pub(super) fn select_physical_device(
    instance: &ash::Instance,
    surface_loader: &khr::surface::Instance,
    surface: vk::SurfaceKHR,
) -> Result<(vk::PhysicalDevice, QueueFamilyIndices), VulkanError> {
    log::info!("Selecting first suitable physical device");

    let physical_devices = get_available_physical_devices(instance)?;

    for physical_device in physical_devices {
        if let Some(indices) = is_device_suitable(instance, physical_device, surface_loader, surface)
        {
            return Ok((physical_device, indices));
        }
    }

    log::error!("No suitable Vulkan device found");
    Err(VulkanError::UnsuitablePhysicalDevice)
}

pub(super) fn create_logical_device(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
    indices: &QueueFamilyIndices,
) -> Result<(ash::Device, vk::Queue, vk::Queue, vk::Queue), VulkanError> {
    log::info!("Creating logical device");

    let queue_priorities = [1.0f32];

    // One queue per *distinct* family: graphics, compute and transfer may all coincide or overlap
    // in any combination, and every family fetched below must have been created here.
    let mut unique_families = vec![indices.graphics];
    for family in [indices.transfer, indices.compute] {
        if !unique_families.contains(&family) {
            unique_families.push(family);
        }
    }

    let queue_create_infos: Vec<_> = unique_families
        .iter()
        .map(|&family| {
            vk::DeviceQueueCreateInfo::default()
                .queue_family_index(family)
                .queue_priorities(&queue_priorities)
        })
        .collect();

    let extensions = get_required_device_extensions();

    // TAG: Device features
    let mut features13 = vk::PhysicalDeviceVulkan13Features::default()
        .dynamic_rendering(true)
        .synchronization2(true);

    let mut features12 = vk::PhysicalDeviceVulkan12Features::default()
        .buffer_device_address(true)
        .descriptor_indexing(true)
        .shader_sampled_image_array_non_uniform_indexing(true)
        .runtime_descriptor_array(true)
        .descriptor_binding_variable_descriptor_count(true)
        .descriptor_binding_partially_bound(true);

    let mut features11 = vk::PhysicalDeviceVulkan11Features::default().shader_draw_parameters(true);

    let features = vk::PhysicalDeviceFeatures::default()
        .sampler_anisotropy(true)
        .shader_int64(true);

    let mut features2 = vk::PhysicalDeviceFeatures2::default()
        .features(features)
        .push_next(&mut features11)
        .push_next(&mut features12)
        .push_next(&mut features13);

    let device_create_info = vk::DeviceCreateInfo::default()
        .queue_create_infos(&queue_create_infos)
        .enabled_extension_names(&extensions)
        .push_next(&mut features2);

    let device = unsafe {
        instance
            .create_device(physical_device, &device_create_info, None)
            .map_err(|e| VulkanError::LogicalDeviceCreationError(e))?
    };

    let graphics_queue = unsafe { device.get_device_queue(indices.graphics, 0) };
    let compute_queue = unsafe { device.get_device_queue(indices.compute, 0) };
    let transfer_queue = unsafe { device.get_device_queue(indices.transfer, 0) };

    Ok((device, graphics_queue, compute_queue, transfer_queue))
}
