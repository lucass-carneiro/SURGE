use crate::config::EngineConfig;
use crate::errors::VulkanError;
use log;
use std::default::Default;
use std::sync::Arc;
#[cfg(feature = "validation_layers")]
use vulkano::instance::debug::DebugUtilsMessenger;
use vulkano::{
    Version, VulkanLibrary,
    device::{
        Device, DeviceCreateInfo, DeviceExtensions, DeviceFeatures, Queue, QueueCreateInfo,
        QueueFamilyProperties, QueueFlags, physical::PhysicalDevice,
    },
    format::Format,
    image::{Image, ImageUsage},
    instance::{
        Instance, InstanceCreateInfo, InstanceExtensions, LayerProperties,
        debug::{
            DebugUtilsMessageSeverity, DebugUtilsMessageType, DebugUtilsMessengerCallback,
            DebugUtilsMessengerCallbackData, DebugUtilsMessengerCreateInfo,
        },
    },
    swapchain::{ColorSpace, CompositeAlpha, PresentMode, Surface, Swapchain, SwapchainCreateInfo},
    sync::Sharing,
};
use winit::{event_loop::ActiveEventLoop, window::Window};

#[derive(Debug)]
struct QueueFamilyIndices {
    graphics: u32,
    compute: u32,
    transfer: u32,
}

#[derive(Debug)]
pub struct VulkanContext {
    instance: Arc<Instance>,
    dbg_msg: DebugUtilsMessenger,
    physical_device: Arc<PhysicalDevice>,

    device: Arc<Device>,
    queues: Vec<Arc<Queue>>,

    surface: Arc<Surface>,
    swapchain: Arc<Swapchain>,
    swapchain_images: Vec<Arc<Image>>,
}

fn get_required_instance_extensions(event_loop: &ActiveEventLoop) -> InstanceExtensions {
    log::info!("Querying required Vulkan instance extensions");

    // Window extensions
    let mut ext = Surface::required_extensions(event_loop).unwrap();

    //Debug handler
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

fn get_available_physical_devices(
    instance: &Arc<Instance>,
) -> Result<Vec<Arc<PhysicalDevice>>, VulkanError> {
    match instance.enumerate_physical_devices() {
        Ok(o) => Ok(o.collect()),
        Err(e) => {
            log::error!("Unable to list available Vulkan physical devices: {}", e);
            Err(VulkanError::PhysicalDeviceListError(e))
        }
    }
}

fn device_has_required_features(physical_device: &Arc<PhysicalDevice>) -> bool {
    let features = physical_device.supported_features();
    features.buffer_device_address
        && features.descriptor_indexing
        && features.shader_sampled_image_array_non_uniform_indexing
        && features.runtime_descriptor_array
        && features.descriptor_binding_variable_descriptor_count
        && features.descriptor_binding_partially_bound
        && features.dynamic_rendering
        && features.synchronization2
}

fn get_required_device_extensions() -> DeviceExtensions {
    let mut ext = DeviceExtensions::empty();
    ext.khr_swapchain = true;
    ext.khr_dynamic_rendering = true;
    ext
}

fn device_has_required_extensions(physical_device: &Arc<PhysicalDevice>) -> bool {
    let av_dev_ext = physical_device.supported_extensions();
    let rq_dev_ext = get_required_device_extensions();
    av_dev_ext.contains(&rq_dev_ext)
}

fn get_required_device_queue_families() -> Vec<QueueFlags> {
    vec![
        QueueFlags::GRAPHICS,
        QueueFlags::TRANSFER,
        QueueFlags::COMPUTE,
    ]
}

fn device_has_required_queue_families(physical_device: &Arc<PhysicalDevice>) -> bool {
    let av_queue_families = physical_device.queue_family_properties();
    let rq_queue_families = get_required_device_queue_families();

    for rq_family in rq_queue_families {
        let mut found = false;

        for av_queue_family in av_queue_families {
            if av_queue_family.queue_flags.contains(rq_family) {
                found = true;
                break;
            }
            log::warn!("av {:?}", av_queue_family);
        }

        if !found {
            log::warn!("{:?}", rq_family);
            return false;
        }
    }

    true
}

fn get_queue_family_indices(queue_families: &[QueueFamilyProperties]) -> QueueFamilyIndices {
    let mut indices = QueueFamilyIndices {
        graphics: 0,
        compute: 0,
        transfer: 0,
    };

    let mut i = 0u32;

    for family in queue_families {
        match family.queue_flags {
            QueueFlags::GRAPHICS => indices.graphics = i,
            QueueFlags::TRANSFER => indices.transfer = i,
            QueueFlags::COMPUTE => indices.compute = i,
            _ => (),
        }

        i += 1;
    }

    indices
}

fn is_device_suitable(physical_device: &Arc<PhysicalDevice>) -> bool {
    log::info!(
        "Checking device suitability of {}",
        physical_device.properties().device_name
    );

    let has_required_features = device_has_required_features(physical_device);
    let has_device_extensions = device_has_required_extensions(physical_device);
    let has_queue_familes = device_has_required_queue_families(physical_device);

    let is_suitable = has_required_features && has_device_extensions && has_queue_familes;

    if is_suitable {
        log::info!("Device is suitable");
    } else {
        log::info!(
            "Device is unsuitable:
  Has required features? {}
  Has required device extensions? {}
  Has required queue families? {}",
            has_required_features,
            has_device_extensions,
            has_queue_familes
        );
    }

    is_suitable
}

fn select_physical_device(instance: &Arc<Instance>) -> Result<Arc<PhysicalDevice>, VulkanError> {
    log::info!("Selecting first suitable physical device");

    let physical_devices = get_available_physical_devices(instance)?;

    for physical_device in physical_devices {
        if is_device_suitable(&physical_device) {
            return Ok(physical_device.clone());
        }
    }

    log::error!("No suitable vulkan device found");
    Err(VulkanError::UnsuitablePhysicalDevice)
}

fn get_required_device_features() -> DeviceFeatures {
    let mut features = DeviceFeatures::empty();
    features.dynamic_rendering = true;
    features.synchronization2 = true;
    features.buffer_device_address = true;
    features.descriptor_indexing = true;
    features.shader_sampled_image_array_non_uniform_indexing = true;
    features.runtime_descriptor_array = true;
    features.descriptor_binding_variable_descriptor_count = true;
    features.descriptor_binding_partially_bound = true;
    features
}

fn create_logical_device(
    physical_device: &Arc<PhysicalDevice>,
) -> Result<(Arc<Device>, Vec<Arc<Queue>>), VulkanError> {
    log::info!("Creating logical device");

    // Extensions and features
    let features = get_required_device_features();
    let extensions = get_required_device_extensions();

    // Queues. We need at least a graphics queue. The other queues are optional
    let queue_families = physical_device.queue_family_properties();
    let indices = get_queue_family_indices(queue_families);

    let mut queue_cis: Vec<QueueCreateInfo> = Vec::new();

    // The required graphics queue
    queue_cis.push(QueueCreateInfo {
        queue_family_index: indices.graphics,
        ..Default::default()
    });

    // Optional queues
    if indices.transfer != indices.graphics && indices.transfer != indices.compute {
        queue_cis.push(QueueCreateInfo {
            queue_family_index: indices.transfer,
            ..Default::default()
        });
    }

    if indices.compute != indices.graphics && indices.compute != indices.transfer {
        queue_cis.push(QueueCreateInfo {
            queue_family_index: indices.transfer,
            ..Default::default()
        });
    }

    // Device creation
    let device_ci = DeviceCreateInfo {
        queue_create_infos: queue_cis,
        enabled_extensions: extensions,
        enabled_features: features,
        ..Default::default()
    };

    let (device, queues_it) = match Device::new(physical_device.clone(), device_ci) {
        Ok(o) => o,
        Err(e) => {
            log::error!("Unable to create vulkan device: {}", e);
            return Err(VulkanError::LogicalDeviceCreationError(e.unwrap()));
        }
    };

    Ok((device, queues_it.collect()))
}

fn create_swapchain(
    physical_device: &Arc<PhysicalDevice>,
    device: &Arc<Device>,
    surface: &Arc<Surface>,
    width: u32,
    height: u32,
    vsync: bool,
) -> Result<(Arc<Swapchain>, Vec<Arc<Image>>), VulkanError> {
    log::info!("Creating swapchain");

    // Surface capabilities
    let surface_capabilities =
        match physical_device.surface_capabilities(surface, Default::default()) {
            Ok(o) => o,
            Err(e) => {
                log::error!("Unable to query device surface capabilities {}", e);
                return Err(VulkanError::SurfaceCapabilityQueryError(e.unwrap()));
            }
        };

    // Clamp extents to make sure they fit the device capabilities
    let img_extent = [
        u32::clamp(
            width,
            surface_capabilities.min_image_extent[0],
            surface_capabilities.min_image_extent[0],
        ),
        u32::clamp(
            height,
            surface_capabilities.min_image_extent[1],
            surface_capabilities.min_image_extent[1],
        ),
    ];

    // Set image formats
    let img_format = Format::B8G8R8A8_UNORM;
    let img_colorspace = ColorSpace::SrgbNonLinear;
    let img_usage = ImageUsage::TRANSFER_DST | ImageUsage::COLOR_ATTACHMENT;

    // Set image count
    let image_count = surface_capabilities
        .max_image_count
        .unwrap_or(surface_capabilities.min_image_count + 1);

    // Set presentation mode
    let present_mode = if vsync {
        PresentMode::Fifo
    } else {
        PresentMode::Immediate
    };

    // Creation
    let swpc_ci = SwapchainCreateInfo {
        min_image_count: image_count,
        image_format: img_format,
        image_color_space: img_colorspace,
        image_extent: img_extent,
        image_array_layers: 1,
        image_usage: img_usage,
        image_sharing: Sharing::Exclusive,
        pre_transform: surface_capabilities.current_transform,
        composite_alpha: CompositeAlpha::Opaque,
        present_mode: present_mode,
        clipped: true,
        ..Default::default()
    };

    match Swapchain::new(device.clone(), surface.clone(), swpc_ci) {
        Ok(o) => Ok(o),
        Err(e) => {
            log::error!("Unable to create swapchain: {}", e);
            return Err(VulkanError::SwapchainCreationError(e.unwrap()));
        }
    }
}

impl VulkanContext {
    pub fn new(
        event_loop: &ActiveEventLoop,
        window: &Arc<Window>,
        config: &EngineConfig,
    ) -> Result<Self, VulkanError> {
        log::info!("Initializing Vulkan");

        // API version
        let library = match VulkanLibrary::new() {
            Ok(o) => o,
            Err(e) => {
                log::error!("Unable to load Vulkan library: {}", e);
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

        // Debug msg info
        #[cfg(feature = "validation_layers")]
        let dbg_msg_ci = dbg_msg_create_info();

        // Instance
        #[cfg(feature = "validation_layers")]
        let instance = build_instance(
            library,
            supported_api_version,
            required_validation_layers,
            required_instance_extensions,
            dbg_msg_ci.clone(),
        )?;
        #[cfg(not(feature = "validation_layers"))]
        let instance =
            build_instance(library, supported_api_version, required_instance_extensions)?;

        // Debug msg
        #[cfg(feature = "validation_layers")]
        let dbg_msg = match DebugUtilsMessenger::new(instance.clone(), dbg_msg_ci.clone()) {
            Ok(o) => o,
            Err(e) => {
                log::error!("Unable to create Vulkan debug messenger: {}", e);
                return Err(VulkanError::DebugMessengerCreationError(e.unwrap()));
            }
        };

        // Select physical device
        let physical_device = select_physical_device(&instance)?;

        // Create logical device and queues
        let (device, queues) = create_logical_device(&physical_device)?;

        // Get window surface
        let surface = match Surface::from_window(instance.clone(), window.clone()) {
            Ok(o) => o,
            Err(e) => {
                log::error!("Unable to obtain Vulkan surface from window");
                return Err(VulkanError::SurfaceCreationError(e));
            }
        };

        // Create Swapchain
        let (swapchain, swapchain_images) = create_swapchain(
            &physical_device,
            &device,
            &surface,
            config.resolution.width,
            config.resolution.height,
            config.renderer.vsync,
        )?;

        Ok(Self {
            instance,
            dbg_msg,
            physical_device,
            device,
            queues,
            surface,
            swapchain,
            swapchain_images,
        })
    }
}
