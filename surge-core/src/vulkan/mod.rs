use crate::errors::VulkanError;
use log;
use std::default::Default;
use std::sync::Arc;
use vulkano::{
    Version, VulkanLibrary,
    instance::{
        Instance, InstanceCreateInfo, InstanceExtensions, LayerProperties,
        debug::{
            DebugUtilsMessageSeverity, DebugUtilsMessageType, DebugUtilsMessengerCallback,
            DebugUtilsMessengerCallbackData, DebugUtilsMessengerCreateInfo,
        },
    },
    swapchain::Surface,
};
use winit::event_loop::EventLoop;

#[derive(Debug)]
pub struct VulkanContext {
    instance: Arc<Instance>,
}

fn get_required_instance_extensions(event_loop: &EventLoop<()>) -> InstanceExtensions {
    log::info!("Querying required Vulkan instance extensions");

    // Window extensions
    let mut ext = Surface::required_extensions(event_loop);

    // Debug handler (if validation layers are available)
    #[cfg(feature = "validation_layers")]
    {
        ext.ext_debug_utils = true;
    }

    ext
}

#[cfg(feature = "validation_layers")]
fn get_required_validation_layers(
    library: &Arc<VulkanLibrary>,
) -> Result<Vec<String>, VulkanError> {
    log::info!("Cheking available validation layers");

    let av_layers: Vec<LayerProperties> = match library.layer_properties() {
        Ok(o) => o,
        Err(e) => {
            log::error!("Unable to query available Vulkan validation layers: {}", e);
            return Err(VulkanError::ValidationLayerQueryError(e));
        }
    }
    .collect();

    // The list of required layer names goes here
    let req_layers = vec!["VK_LAYER_KHRONOS_validation".to_string()];

    for req_layer in &req_layers {
        let mut found = false;

        for av_layer in &av_layers {
            if req_layer == av_layer.name() {
                found = true;
                break;
            }
        }

        if !found {
            return Err(VulkanError::ValidationLayerNotFound(req_layer.to_string()));
        }
    }

    Ok(req_layers)
}

#[cfg(feature = "validation_layers")]
fn dbg_msg_callback(
    message_severity: DebugUtilsMessageSeverity,
    message_type: DebugUtilsMessageType,
    callback_data: DebugUtilsMessengerCallbackData<'_>,
) {
    let verbose_or_info = message_severity == DebugUtilsMessageSeverity::VERBOSE
        || message_severity == DebugUtilsMessageSeverity::INFO;

    let warning = message_severity == DebugUtilsMessageSeverity::WARNING;

    if verbose_or_info {
        match message_type {
            DebugUtilsMessageType::GENERAL => {
                log::info!("Vulkan info (General): {}", callback_data.message)
            }
            DebugUtilsMessageType::VALIDATION => {
                log::info!("Vulkan info (Validation): {}", callback_data.message)
            }
            DebugUtilsMessageType::PERFORMANCE => {
                log::info!("Vulkan info (Performance): {}", callback_data.message)
            }
            _ => {
                log::info!("Vulkan info (Unknown): {}", callback_data.message)
            }
        }
    } else if warning {
        match message_type {
            DebugUtilsMessageType::GENERAL => {
                log::warn!("Vulkan warning (General): {}", callback_data.message)
            }
            DebugUtilsMessageType::VALIDATION => {
                log::warn!("Vulkan warning (Validation): {}", callback_data.message)
            }
            DebugUtilsMessageType::PERFORMANCE => {
                log::warn!("Vulkan warning (Performance): {}", callback_data.message)
            }
            _ => {
                log::warn!("Vulkan warning (Unknown): {}", callback_data.message)
            }
        }
    } else {
        match message_type {
            DebugUtilsMessageType::GENERAL => {
                log::error!("Vulkan error (General): {}", callback_data.message)
            }
            DebugUtilsMessageType::VALIDATION => {
                log::error!("Vulkan error (Validation): {}", callback_data.message)
            }
            DebugUtilsMessageType::PERFORMANCE => {
                log::error!("Vulkan error (Performance): {}", callback_data.message)
            }
            _ => {
                log::error!("Vulkan error (Unknown): {}", callback_data.message)
            }
        }
    }
}

#[cfg(feature = "validation_layers")]
fn dbg_msg_create_info() -> DebugUtilsMessengerCreateInfo {
    let mut ci = DebugUtilsMessengerCreateInfo::user_callback(unsafe {
        DebugUtilsMessengerCallback::new(dbg_msg_callback)
    });

    ci.message_severity = DebugUtilsMessageSeverity::INFO
        | DebugUtilsMessageSeverity::VERBOSE
        | DebugUtilsMessageSeverity::WARNING
        | DebugUtilsMessageSeverity::ERROR;

    ci.message_type = DebugUtilsMessageType::GENERAL
        | DebugUtilsMessageType::VALIDATION
        | DebugUtilsMessageType::PERFORMANCE;

    ci
}

#[cfg(feature = "validation_layers")]
fn build_instance(
    library: Arc<VulkanLibrary>,
    api_version: Version,
    layers: Vec<String>,
    extensions: InstanceExtensions,
    dbg_msg_ci: DebugUtilsMessengerCreateInfo,
) -> Result<Arc<Instance>, VulkanError> {
    log::info!("Creating Vulkan instance");

    let ci = InstanceCreateInfo {
        application_name: Some("SURGE Player".to_string()),
        application_version: Version::major_minor(1, 4),
        engine_name: Some("SURGE".to_string()),
        engine_version: Version::major_minor(1, 4),
        max_api_version: Some(api_version),
        enabled_layers: layers,
        enabled_extensions: extensions,
        debug_utils_messengers: vec![dbg_msg_ci],
        ..Default::default()
    };

    match Instance::new(library, ci) {
        Ok(o) => Ok(o),
        Err(e) => {
            log::error!("Unable  to create Vulkan instance: {}", e);
            Err(VulkanError::InstanceCreationError(e.unwrap()))
        }
    }
}

#[cfg(not(feature = "validation_layers"))]
fn build_instance(
    library: Arc<VulkanLibrary>,
    api_version: Version,
    extensions: InstanceExtensions,
) -> Result<Arc<Instance>, VulkanError> {
    log::info!("Creating Vulkan instance");

    let ci = InstanceCreateInfo {
        application_name: Some("SURGE Player".to_string()),
        application_version: Version::major_minor(1, 4),
        engine_name: Some("SURGE".to_string()),
        engine_version: Version::major_minor(1, 4),
        max_api_version: Some(api_version),
        enabled_extensions: extensions,
        ..Default::default()
    };

    match Instance::new(library, ci) {
        Ok(o) => Ok(o),
        Err(e) => {
            log::error!("Unable  to create Vulkan instance: {}", e);
            Err(VulkanError::InstanceCreationError(e.unwrap()))
        }
    }
}

impl VulkanContext {
    pub fn new(event_loop: &EventLoop<()>) -> Result<Self, VulkanError> {
        log::info!("Initializing Vulkan");

        // API version
        let library = match VulkanLibrary::new() {
            Ok(o) => o,
            Err(e) => {
                log::error!("{}", e);
                return Err(VulkanError::LibraryLoadingError(e));
            }
        };

        let supported_api_version = library.api_version();
        log::info!("Vulkan supported API version: {}", supported_api_version);

        // Instance extensions
        let required_instance_extensions = get_required_instance_extensions(event_loop);

        // Validation layers
        #[cfg(feature = "validation_layers")]
        let required_validation_layers = get_required_validation_layers(&library)?;

        // Debug msg
        #[cfg(feature = "validation_layers")]
        let dbg_msg_ci = dbg_msg_create_info();

        // Instance
        #[cfg(feature = "validation_layers")]
        let instance = build_instance(
            library,
            supported_api_version,
            required_validation_layers,
            required_instance_extensions,
            dbg_msg_ci,
        )?;
        #[cfg(not(feature = "validation_layers"))]
        let instance =
            build_instance(library, supported_api_version, required_instance_extensions)?;

        Ok(Self { instance })
    }
}
