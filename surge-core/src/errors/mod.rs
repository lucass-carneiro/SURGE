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
    #[error("Unable to load Vulkan library: {0}")]
    LibraryLoadingError(ash::LoadingError),

    #[cfg(feature = "validation_layers")]
    #[error("Unable to query available Vulkan validation layers {0}")]
    ValidationLayerQueryError(ash::vk::Result),

    #[cfg(feature = "validation_layers")]
    #[error("Validation layer {0} is not available")]
    ValidationLayerNotFound(String),

    #[error("Unable to create Vulkan instance: {0}")]
    InstanceCreationError(ash::vk::Result),

    #[error("Unable to create Vulkan debug messenger: {0}")]
    DebugMessengerCreationError(ash::vk::Result),

    #[error("Unable to list available Vulkan physical devices: {0}")]
    PhysicalDeviceListError(ash::vk::Result),

    #[error("Unable to find suitable Vulkan physical device")]
    UnsuitablePhysicalDevice,

    #[error("Unable to create Vulkan device: {0}")]
    LogicalDeviceCreationError(ash::vk::Result),

    #[error("Unable to create Vulkan window surface: {0}")]
    SurfaceCreationError(ash::vk::Result),

    #[error("Unable to query Vulkan device surface capabilities: {0}")]
    SurfaceCapabilityQueryError(ash::vk::Result),

    #[error("Unable to create swapchain: {0}")]
    SwapchainCreationError(ash::vk::Result),

    #[error("Unable to create command pool: {0}")]
    CommandPoolCreation(ash::vk::Result),

    #[error("Unable to allocate command buffer: {0}")]
    CommandBufferAllocation(ash::vk::Result),

    #[error("Unable to create semaphore: {0}")]
    SemaphoreCreationError(ash::vk::Result),

    #[error("Unable to create fence: {0}")]
    FenceCreationError(ash::vk::Result),
}
