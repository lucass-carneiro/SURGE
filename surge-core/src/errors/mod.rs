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

    #[error("Unable to recreate swapchain: {0}")]
    SwapchainRecreationError(ash::vk::Result),

    #[error("Unable to destroy swapchain: {0}")]
    SwapchainDestructionError(ash::vk::Result),

    #[error("Unable to present frame {0} swapchain: {1}")]
    SwapchainPresentError(usize, ash::vk::Result),

    #[error("Unable to create depth buffer {0}")]
    DepthImageCreationError(ash::vk::Result),

    #[error("Unable to create command pool: {0}")]
    CommandPoolCreation(ash::vk::Result),

    #[error("Unable to allocate command buffer: {0}")]
    CommandBufferAllocation(ash::vk::Result),

    #[error("Unable to create memory allocator: {0}")]
    MemoryAllocatorCreationError(ash::vk::Result),

    #[error("Unable to create semaphore: {0}")]
    SemaphoreCreationError(ash::vk::Result),

    #[error("Unable to create fence: {0}")]
    FenceCreationError(ash::vk::Result),

    #[error("Unable to wait fence: {0}")]
    FenceWaiteError(ash::vk::Result),

    #[error("Unable to reset fence: {0}")]
    FenceResetError(ash::vk::Result),

    #[error("Unable to acquire next swapchain image: {0}")]
    SwapchainAcquireError(ash::vk::Result),

    #[error("Unable to reset frame {0} command buffer: {1}")]
    FrameCommandBufferReset(usize, ash::vk::Result),

    #[error("Unable to begin recording frame {0} command buffer: {1}")]
    FrameCommandBufferRecordBeginError(usize, ash::vk::Result),

    #[error("Unable to end recording frame {0} command buffer: {1}")]
    FrameCommandBufferRecordEndError(usize, ash::vk::Result),

    #[error("Unable to submit frame {0} command buffer: {1}")]
    FrameCommandBufferSubmitError(usize, ash::vk::Result),

    #[error("Unable to create image on device: {0}")]
    ImageCreationError(ash::vk::Result),

    #[error("Unable to read shader module: {0}")]
    ShaderModuleReadError(std::io::Error),

    #[error("The shader module does {0} not have a size multiple of u32. Module size: {1} bytes")]
    ShaderModuleInvalidSizeError(String, usize),

    #[error("Unable to create shader module: {0}")]
    ShaderModuleCreateError(ash::vk::Result),

    #[error("Unable to create graphics pipeline: {0}")]
    GraphicsPipelineCreationError(ash::vk::Result),
}
