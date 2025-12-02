use crate::config::EngineConfig;
use crate::errors::VulkanError;
use ash::vk::{
    AccessFlags2, AttachmentLoadOp, AttachmentStoreOp, ClearColorValue, CommandBufferBeginInfo,
    CommandBufferResetFlags, CommandBufferUsageFlags, DependencyInfo, Fence, ImageAspectFlags,
    ImageLayout, ImageMemoryBarrier2, ImageSubresourceRange, ImageViewCreateInfo, ImageViewType,
    Offset2D, PipelineStageFlags, PipelineStageFlags2, PresentInfoKHR, Rect2D,
    RenderingAttachmentInfo, RenderingInfo, SubmitInfo,
};
use ash::{Entry, ext, khr, vk};
use log;
use std::ffi::{CStr, CString};
use std::os::raw::c_void;
use winit::{
    event_loop::ActiveEventLoop,
    raw_window_handle::{HasDisplayHandle, HasWindowHandle},
    window::Window,
};

#[derive(Debug)]
struct QueueFamilyIndices {
    graphics: u32,
    compute: u32,
    transfer: u32,
}

#[derive(Debug)]
pub struct SwapchainImageData {
    pub index: u32,
    pub suboptimal: bool,
}

struct SwapchainData {
    swapchain_loader: khr::swapchain::Device,
    swapchain: vk::SwapchainKHR,
    images: Vec<vk::Image>,
    image_views: Vec<vk::ImageView>,
    format: vk::Format,
    extent: vk::Extent2D,
}

struct SwpcLayoutTransitionInfo {
    pub image_index: u32,
    pub old_layout: ImageLayout,
    pub new_layout: ImageLayout,
    pub src_access_mask: AccessFlags2,
    pub dst_access_mask: AccessFlags2,
    pub src_stage_mask: PipelineStageFlags2,
    pub dst_stage_mask: PipelineStageFlags2,
}

const FRAMES_IN_FLIGHT: usize = 2;

pub struct VulkanContext {
    entry: Entry,
    instance: ash::Instance,
    #[cfg(feature = "validation_layers")]
    debug_utils: ext::debug_utils::Instance,
    #[cfg(feature = "validation_layers")]
    debug_messenger: vk::DebugUtilsMessengerEXT,
    physical_device: vk::PhysicalDevice,

    device: ash::Device,

    graphics_queue: vk::Queue,
    compute_queue: vk::Queue,
    transfer_queue: vk::Queue,

    surface: khr::surface::Instance,
    surface_khr: vk::SurfaceKHR,

    swapchain_data: SwapchainData,

    command_pool: vk::CommandPool,
    command_buffers: Vec<vk::CommandBuffer>,

    present_completed_sem: Vec<vk::Semaphore>,
    render_finished_sem: Vec<vk::Semaphore>,
    frame_fences: Vec<vk::Fence>,

    current_frame: usize,
    semaphore_index: usize,
}

fn get_required_instance_extensions(
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
fn get_required_validation_layers(entry: &Entry) -> Result<Vec<CString>, VulkanError> {
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
unsafe extern "system" fn debug_callback(
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
fn build_instance(
    entry: &Entry,
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
fn build_instance(entry: &Entry, extensions: &[*const i8]) -> Result<ash::Instance, VulkanError> {
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

fn get_available_physical_devices(
    instance: &ash::Instance,
) -> Result<Vec<vk::PhysicalDevice>, VulkanError> {
    unsafe {
        instance
            .enumerate_physical_devices()
            .map_err(|e| VulkanError::PhysicalDeviceListError(e))
    }
}

fn device_has_required_features(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> bool {
    let mut features13 = vk::PhysicalDeviceVulkan13Features::default();
    let mut features12 = vk::PhysicalDeviceVulkan12Features::default();
    features12.p_next = &mut features13 as *mut _ as *mut c_void;

    let mut features2 = vk::PhysicalDeviceFeatures2::default();
    features2.p_next = &mut features12 as *mut _ as *mut c_void;

    unsafe {
        instance.get_physical_device_features2(physical_device, &mut features2);
    }

    features12.buffer_device_address == vk::TRUE
        && features12.descriptor_indexing == vk::TRUE
        && features12.shader_sampled_image_array_non_uniform_indexing == vk::TRUE
        && features12.runtime_descriptor_array == vk::TRUE
        && features12.descriptor_binding_variable_descriptor_count == vk::TRUE
        && features12.descriptor_binding_partially_bound == vk::TRUE
        && features13.dynamic_rendering == vk::TRUE
        && features13.synchronization2 == vk::TRUE
}

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

fn get_queue_family_indices(queue_families: &[vk::QueueFamilyProperties]) -> QueueFamilyIndices {
    let mut indices = QueueFamilyIndices {
        graphics: 0,
        compute: 0,
        transfer: 0,
    };

    for (i, family) in queue_families.iter().enumerate() {
        if family.queue_flags.contains(vk::QueueFlags::GRAPHICS) {
            indices.graphics = i as u32;
        }
        if family.queue_flags.contains(vk::QueueFlags::COMPUTE) {
            indices.compute = i as u32;
        }
        if family.queue_flags.contains(vk::QueueFlags::TRANSFER) {
            indices.transfer = i as u32;
        }
    }

    indices
}

fn is_device_suitable(instance: &ash::Instance, physical_device: vk::PhysicalDevice) -> bool {
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

    let is_suitable = has_required_features && has_device_extensions && has_queue_families;

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
            has_queue_families
        );
    }

    is_suitable
}

fn select_physical_device(instance: &ash::Instance) -> Result<vk::PhysicalDevice, VulkanError> {
    log::info!("Selecting first suitable physical device");

    let physical_devices = get_available_physical_devices(instance)?;

    for physical_device in physical_devices {
        if is_device_suitable(instance, physical_device) {
            return Ok(physical_device);
        }
    }

    log::error!("No suitable Vulkan device found");
    Err(VulkanError::UnsuitablePhysicalDevice)
}

fn create_logical_device(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> Result<(ash::Device, vk::Queue, vk::Queue, vk::Queue), VulkanError> {
    log::info!("Creating logical device");

    let queue_families =
        unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
    let indices = get_queue_family_indices(&queue_families);

    let queue_priorities = [1.0f32];
    let mut queue_create_infos = vec![
        vk::DeviceQueueCreateInfo::default()
            .queue_family_index(indices.graphics)
            .queue_priorities(&queue_priorities),
    ];

    // Add optional dedicated transfer queue if different from graphics
    if indices.transfer != indices.graphics && indices.transfer != indices.compute {
        queue_create_infos.push(
            vk::DeviceQueueCreateInfo::default()
                .queue_family_index(indices.transfer)
                .queue_priorities(&queue_priorities),
        );
    }

    // Add optional dedicated compute queue if different from graphics and transfer
    if indices.compute != indices.graphics && indices.compute != indices.transfer {
        queue_create_infos.push(
            vk::DeviceQueueCreateInfo::default()
                .queue_family_index(indices.compute)
                .queue_priorities(&queue_priorities),
        );
    }

    let extensions = get_required_device_extensions();

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
    features12.p_next = &mut features13 as *mut _ as *mut c_void;

    let mut features2 = vk::PhysicalDeviceFeatures2::default();
    features2.p_next = &mut features12 as *mut _ as *mut c_void;

    let mut device_create_info = vk::DeviceCreateInfo::default()
        .queue_create_infos(&queue_create_infos)
        .enabled_extension_names(&extensions);
    device_create_info.p_next = &mut features2 as *mut _ as *mut c_void;

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

fn create_swapchain(
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
        let sr = ImageSubresourceRange {
            aspect_mask: ImageAspectFlags::COLOR,
            base_mip_level: 0,
            level_count: 1,
            base_array_layer: 0,
            layer_count: 1,
            ..Default::default()
        };

        let ivci = ImageViewCreateInfo {
            view_type: ImageViewType::TYPE_2D,
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

fn create_command_pool(
    device: &ash::Device,
    queue_family_index: u32,
) -> Result<vk::CommandPool, VulkanError> {
    let create_info = vk::CommandPoolCreateInfo::default()
        .flags(vk::CommandPoolCreateFlags::RESET_COMMAND_BUFFER)
        .queue_family_index(queue_family_index);

    unsafe {
        device
            .create_command_pool(&create_info, None)
            .map_err(|e| VulkanError::CommandPoolCreation(e))
    }
}

fn create_command_buffers(
    device: &ash::Device,
    command_pool: vk::CommandPool,
) -> Result<Vec<vk::CommandBuffer>, VulkanError> {
    let allocate_info = vk::CommandBufferAllocateInfo::default()
        .command_pool(command_pool)
        .level(vk::CommandBufferLevel::PRIMARY)
        .command_buffer_count(FRAMES_IN_FLIGHT as u32);

    unsafe {
        device
            .allocate_command_buffers(&allocate_info)
            .map_err(|e| VulkanError::CommandBufferAllocation(e))
    }
}

fn create_semaphores(
    device: &ash::Device,
    swpc_images: usize,
) -> Result<Vec<vk::Semaphore>, VulkanError> {
    let mut semaphores = Vec::new();
    let create_info = vk::SemaphoreCreateInfo::default();

    for _ in 0..swpc_images {
        let semaphore = unsafe {
            device
                .create_semaphore(&create_info, None)
                .map_err(|e| VulkanError::SemaphoreCreationError(e))?
        };
        semaphores.push(semaphore);
    }

    Ok(semaphores)
}

fn create_fences(device: &ash::Device) -> Result<Vec<vk::Fence>, VulkanError> {
    let mut fences = Vec::new();
    let create_info = vk::FenceCreateInfo::default().flags(vk::FenceCreateFlags::SIGNALED);

    for _ in 0..FRAMES_IN_FLIGHT {
        let fence = unsafe {
            device
                .create_fence(&create_info, None)
                .map_err(|e| VulkanError::FenceCreationError(e))?
        };
        fences.push(fence);
    }

    Ok(fences)
}

impl VulkanContext {
    pub fn new(
        event_loop: &ActiveEventLoop,
        window: &Window,
        config: &EngineConfig,
    ) -> Result<Self, VulkanError> {
        log::info!("Initializing Vulkan");

        let entry = unsafe { Entry::load().map_err(|e| VulkanError::LibraryLoadingError(e))? };

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

        let extensions = get_required_instance_extensions(event_loop)?;

        #[cfg(feature = "validation_layers")]
        let layers = get_required_validation_layers(&entry)?;

        #[cfg(feature = "validation_layers")]
        let instance = build_instance(&entry, &extensions, &layers)?;

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
                .pfn_user_callback(Some(debug_callback));

            let messenger = unsafe {
                debug_utils
                    .create_debug_utils_messenger(&debug_create_info, None)
                    .map_err(|e| VulkanError::DebugMessengerCreationError(e))?
            };

            (debug_utils, messenger)
        };

        let physical_device = select_physical_device(&instance)?;

        let (device, graphics_queue, compute_queue, transfer_queue) =
            create_logical_device(&instance, physical_device)?;

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

        let swapchain_data = create_swapchain(
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
        let indices = get_queue_family_indices(&queue_families);

        let command_pool = create_command_pool(&device, indices.graphics)?;
        let command_buffers = create_command_buffers(&device, command_pool)?;

        let present_completed_sem = create_semaphores(&device, swapchain_data.images.len())?;
        let render_finished_sem = create_semaphores(&device, swapchain_data.images.len())?;
        let frame_fences = create_fences(&device)?;

        Ok(Self {
            entry,
            instance,
            #[cfg(feature = "validation_layers")]
            debug_utils,
            #[cfg(feature = "validation_layers")]
            debug_messenger,
            physical_device,
            device,
            graphics_queue,
            compute_queue,
            transfer_queue,
            surface: surface_loader,
            surface_khr,
            swapchain_data,
            command_pool,
            command_buffers,
            present_completed_sem,
            render_finished_sem,
            frame_fences,
            current_frame: 0,
            semaphore_index: 0,
        })
    }

    fn cmd_transition_swpc_image_layout(&self, ti: SwpcLayoutTransitionInfo) {
        let subresource_range = ImageSubresourceRange {
            aspect_mask: ImageAspectFlags::COLOR,
            base_mip_level: 0,
            level_count: 1,
            base_array_layer: 0,
            layer_count: 1,
            ..Default::default()
        };

        let barrier = ImageMemoryBarrier2 {
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

        let dependency_info = DependencyInfo {
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
                    CommandBufferResetFlags::empty(),
                )
                .map_err(|e| VulkanError::FrameCommandBufferReset(self.current_frame, e))
        }?;

        // Begin command recording
        unsafe {
            let bi = CommandBufferBeginInfo {
                flags: CommandBufferUsageFlags::ONE_TIME_SUBMIT,
                ..Default::default()
            };

            self.device
                .begin_command_buffer(self.command_buffers[self.current_frame], &bi)
                .map_err(|e| VulkanError::FrameCommandBufferRecordBeginError(self.current_frame, e))
        }?;

        // Transition swapchain image
        let swpc_ti = SwpcLayoutTransitionInfo {
            image_index: swpc_img_idx,
            old_layout: ImageLayout::UNDEFINED,
            new_layout: ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            src_access_mask: AccessFlags2::empty(),
            dst_access_mask: AccessFlags2::COLOR_ATTACHMENT_WRITE,
            src_stage_mask: PipelineStageFlags2::COLOR_ATTACHMENT_OUTPUT,
            dst_stage_mask: PipelineStageFlags2::COLOR_ATTACHMENT_OUTPUT,
        };

        self.cmd_transition_swpc_image_layout(swpc_ti);

        Ok(())
    }

    pub fn cmd_render_begin(&self, swpc_img_idx: u32, config: &EngineConfig) {
        let ccl = ClearColorValue {
            float32: [
                config.clear_color.r as f32,
                config.clear_color.g as f32,
                config.clear_color.b as f32,
                config.clear_color.a as f32,
            ],
        };

        let rai = RenderingAttachmentInfo {
            image_view: self.swapchain_data.image_views[swpc_img_idx as usize],
            image_layout: ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            load_op: AttachmentLoadOp::CLEAR,
            store_op: AttachmentStoreOp::STORE,
            clear_value: vk::ClearValue { color: ccl },
            ..Default::default()
        };

        let ra = Rect2D {
            offset: Offset2D { x: 0, y: 0 },
            extent: self.swapchain_data.extent,
        };

        let ri = RenderingInfo {
            render_area: ra,
            layer_count: 1,
            color_attachment_count: 1,
            p_color_attachments: &rai,
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
            old_layout: ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
            new_layout: ImageLayout::PRESENT_SRC_KHR,
            src_access_mask: AccessFlags2::COLOR_ATTACHMENT_WRITE,
            dst_access_mask: AccessFlags2::empty(),
            src_stage_mask: PipelineStageFlags2::COLOR_ATTACHMENT_OUTPUT,
            dst_stage_mask: PipelineStageFlags2::BOTTOM_OF_PIPE,
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
        let si = SubmitInfo {
            wait_semaphore_count: 1,
            p_wait_semaphores: &self.present_completed_sem[self.semaphore_index],
            p_wait_dst_stage_mask: &PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT,
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
                    Fence::null(),
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
        let pi = PresentInfoKHR {
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

        self.swapchain_data = create_swapchain(
            &self.instance,
            self.physical_device,
            &self.device,
            &self.surface,
            self.surface_khr,
            config.resolution.width,
            config.resolution.height,
            config.renderer.vsync,
        )?;

        Ok(())
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

            log::debug!("Destroying command pool");
            // Destroy command pool (this also frees command buffers)
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
