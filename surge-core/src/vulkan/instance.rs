use crate::errors::VulkanError;
use ash::{self, ext, vk};
use std::ffi::{CStr, CString, c_void};
use winit::{event_loop::ActiveEventLoop, raw_window_handle::HasDisplayHandle};

pub(super) fn get_required_instance_extensions(
    event_loop: &ActiveEventLoop,
) -> Result<Vec<*const i8>, VulkanError> {
    log::info!("Querying required Vulkan instance extensions");

    // Get required surface extensions from winit
    let mut extensions =
        ash_window::enumerate_required_extensions(event_loop.display_handle().unwrap().as_raw())
            .map_err(|_| VulkanError::UnsuitablePhysicalDevice)?
            .to_vec();

    // Add debug utils extension for validation layers
    #[cfg(feature = "validation_layers")]
    extensions.push(ext::debug_utils::NAME.as_ptr());

    Ok(extensions)
}

#[cfg(feature = "validation_layers")]
pub(super) fn get_required_validation_layers(
    entry: &ash::Entry,
) -> Result<Vec<CString>, VulkanError> {
    log::info!("Checking available validation layers");

    let available_layers = unsafe {
        entry
            .enumerate_instance_layer_properties()
            .map_err(|e| VulkanError::ValidationLayerQueryError(e))?
    };

    let required_layer = CString::new("VK_LAYER_KHRONOS_validation").unwrap();

    let mut found = false;
    for layer in &available_layers {
        let layer_name = unsafe { CStr::from_ptr(layer.layer_name.as_ptr()) };
        if layer_name == required_layer.as_c_str() {
            found = true;
            break;
        }
    }

    if !found {
        return Err(VulkanError::ValidationLayerNotFound(
            required_layer.to_string_lossy().to_string(),
        ));
    }

    Ok(vec![required_layer])
}

#[cfg(feature = "validation_layers")]
pub(super) unsafe extern "system" fn debug_callback(
    message_severity: vk::DebugUtilsMessageSeverityFlagsEXT,
    message_type: vk::DebugUtilsMessageTypeFlagsEXT,
    p_callback_data: *const vk::DebugUtilsMessengerCallbackDataEXT,
    _p_user_data: *mut c_void,
) -> vk::Bool32 {
    unsafe {
        let callback_data = &*p_callback_data;
        let message = CStr::from_ptr(callback_data.p_message).to_string_lossy();

        let type_str = match message_type {
            vk::DebugUtilsMessageTypeFlagsEXT::GENERAL => "General",
            vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION => "Validation",
            vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE => "Performance",
            _ => "Unknown",
        };

        if message_severity.contains(vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE)
            || message_severity.contains(vk::DebugUtilsMessageSeverityFlagsEXT::INFO)
        {
            log::info!("Vulkan info ({}): {}", type_str, message);
        } else if message_severity.contains(vk::DebugUtilsMessageSeverityFlagsEXT::WARNING) {
            log::warn!("Vulkan warning ({}): {}", type_str, message);
        } else {
            log::error!("Vulkan error ({}): {}", type_str, message);
        }

        vk::FALSE
    }
}

#[cfg(feature = "validation_layers")]
pub(super) fn build_instance(
    entry: &ash::Entry,
    extensions: &[*const i8],
    layers: &[CString],
) -> Result<ash::Instance, VulkanError> {
    log::info!("Creating Vulkan instance");

    let app_name = CString::new("SURGE Player").unwrap();
    let engine_name = CString::new("SURGE").unwrap();

    let app_info = vk::ApplicationInfo::default()
        .application_name(&app_name)
        .application_version(vk::make_api_version(0, 1, 4, 0))
        .engine_name(&engine_name)
        .engine_version(vk::make_api_version(0, 1, 4, 0))
        .api_version(vk::API_VERSION_1_3);

    let layer_ptrs: Vec<*const i8> = layers.iter().map(|l| l.as_ptr()).collect();

    let mut debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT::default()
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
        .pfn_user_callback(Some(debug_callback));

    let create_info = vk::InstanceCreateInfo::default()
        .application_info(&app_info)
        .enabled_extension_names(extensions)
        .enabled_layer_names(&layer_ptrs)
        .push_next(&mut debug_create_info);

    unsafe {
        entry
            .create_instance(&create_info, None)
            .map_err(|e| VulkanError::InstanceCreationError(e))
    }
}

#[cfg(not(feature = "validation_layers"))]
pub(super) fn build_instance(
    entry: &Entry,
    extensions: &[*const i8],
) -> Result<ash::Instance, VulkanError> {
    log::info!("Creating Vulkan instance");

    let app_name = CString::new("SURGE Player").unwrap();
    let engine_name = CString::new("SURGE").unwrap();

    let app_info = vk::ApplicationInfo::default()
        .application_name(&app_name)
        .application_version(vk::make_api_version(0, 1, 4, 0))
        .engine_name(&engine_name)
        .engine_version(vk::make_api_version(0, 1, 4, 0))
        .api_version(vk::API_VERSION_1_3);

    let create_info = vk::InstanceCreateInfo::default()
        .application_info(&app_info)
        .enabled_extension_names(extensions);

    unsafe {
        entry
            .create_instance(&create_info, None)
            .map_err(|e| VulkanError::InstanceCreationError(e))
    }
}
