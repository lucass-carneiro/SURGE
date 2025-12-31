use ash::{self, ext, khr, vk};
use std::mem::ManuallyDrop;

const FRAMES_IN_FLIGHT: usize = 2;

#[derive(Debug)]
struct QueueFamilyIndices {
    graphics: u32,
    compute: u32,
    transfer: u32,
}

#[derive(Debug)]
pub(crate) struct AllocatedImage {
    pub image: vk::Image,
    pub image_view: vk::ImageView,
    pub image_memory: vk_mem::Allocation,
}

#[derive(Debug)]
pub struct SwapchainImageData {
    pub index: u32,
    pub suboptimal: bool,
}

struct SwpcLayoutTransitionInfo {
    image_index: u32,
    old_layout: vk::ImageLayout,
    new_layout: vk::ImageLayout,
    src_access_mask: vk::AccessFlags2,
    dst_access_mask: vk::AccessFlags2,
    src_stage_mask: vk::PipelineStageFlags2,
    dst_stage_mask: vk::PipelineStageFlags2,
}

struct SwapchainData {
    swapchain_loader: khr::swapchain::Device,
    swapchain: vk::SwapchainKHR,
    images: Vec<vk::Image>,
    image_views: Vec<vk::ImageView>,
    format: vk::Format,
    extent: vk::Extent2D,
}

const DPETH_FORMAT: vk::Format = vk::Format::D32_SFLOAT;

pub struct VulkanContext {
    entry: ash::Entry,
    instance: ash::Instance,
    #[cfg(feature = "validation_layers")]
    debug_utils: ext::debug_utils::Instance,
    #[cfg(feature = "validation_layers")]
    debug_messenger: vk::DebugUtilsMessengerEXT,
    physical_device: vk::PhysicalDevice,

    memory_allocator: ManuallyDrop<vk_mem::Allocator>,

    device: ash::Device,

    indices: QueueFamilyIndices,
    graphics_queue: vk::Queue,
    compute_queue: vk::Queue,
    transfer_queue: vk::Queue,

    surface: khr::surface::Instance,
    surface_khr: vk::SurfaceKHR,

    swapchain_data: SwapchainData,
    depth_image: AllocatedImage,

    command_pool: vk::CommandPool,
    command_buffers: Vec<vk::CommandBuffer>,

    present_completed_sem: Vec<vk::Semaphore>,
    render_finished_sem: Vec<vk::Semaphore>,
    frame_fences: Vec<vk::Fence>,

    current_frame: usize,
    semaphore_index: usize,
}

mod command;
mod ctx_buffer;
mod ctx_command;
mod ctx_graphics_pipeline;
mod ctx_new_drop;
mod ctx_shader;
mod ctx_swpc;
mod device;
mod image;
mod instance;
pub mod sprite_database;
