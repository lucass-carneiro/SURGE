use libloading;
use std::io;
use thiserror::Error;
use toml;

#[derive(Debug, Error)]
pub enum ModuleError {
    #[error("Unable to load SURGE module {name}: {io_error}")]
    IoError { name: String, io_error: io::Error },

    #[error("Unable to load SURGE module {name}: {lib_error}")]
    LibLoadingError {
        name: String,
        lib_error: libloading::Error,
    },
}

#[derive(Debug, Error)]
pub enum ConfigError {
    #[error("Unable to load SURGE configuration file {name}: {io_error}")]
    IoError { name: String, io_error: io::Error },

    #[error("Unable to parse SURGE configuration file {name}")]
    ParseError {
        name: String,
        error: toml::de::Error,
    },
}

#[derive(Debug, Error)]
pub enum VulkanError {
    #[error("Unable to lod Vulkan library: {0}")]
    LibraryLoadingError(vulkano::LoadingError),

    #[cfg(feature = "validation_layers")]
    #[error("Unable to query available Vulkan validation layers {0}")]
    ValidationLayerQueryError(vulkano::VulkanError),

    #[cfg(feature = "validation_layers")]
    #[error("Validation layer {0} is not available")]
    ValidationLayerNotFound(String),

    #[error("Unable to create Vulkan instance: {0}")]
    InstanceCreationError(vulkano::VulkanError),

    #[error("Unable to create Vulkan debug messenger: {0}")]
    DebugMessengerCreationError(vulkano::VulkanError),

    #[error("Unable to list available Vulkan physical devices: {0}")]
    PhysicalDeviceListError(vulkano::VulkanError),

    #[error("Unable to list find suitable Vulkan physical device")]
    UnsuitablePhysicalDevice,

    #[error("Unable to create Vulkan device: {0}")]
    LogicalDeviceCreationError(vulkano::VulkanError),

    #[error("Unable to create Vulkan window surface: {0}")]
    SurfaceCreationError(vulkano::swapchain::FromWindowError),

    #[error("Unable to query Vulkan device surface capabilities: {0}")]
    SurfaceCapabilityQueryError(vulkano::VulkanError),

    #[error("Unable to create swapchain: {0}")]
    SwapchainCreationError(vulkano::VulkanError),

    #[error("Unable to create command pool: {0}")]
    CommandPoolCreateion(vulkano::VulkanError),

    #[error("Unable to allocate command buffer: {0}")]
    CommandBufferAllocation(vulkano::VulkanError),

    #[error("Unable to create semaphore: {0}")]
    SemaphoreCreationError(vulkano::VulkanError),

    #[error("Unable to create fence: {0}")]
    FenceCreationError(vulkano::VulkanError),
}
