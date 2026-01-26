use super::FRAMES_IN_FLIGHT;
use crate::errors::VulkanError;
use ash::vk;
use vk_mem;

pub(super) fn create_command_pool(
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

pub(super) fn create_command_buffers(
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

pub(super) fn create_semaphores(
    device: &ash::Device,
    count: usize,
) -> Result<Vec<vk::Semaphore>, VulkanError> {
    let mut semaphores = Vec::new();
    let create_info = vk::SemaphoreCreateInfo::default();

    for _ in 0..count {
        let semaphore = unsafe {
            device
                .create_semaphore(&create_info, None)
                .map_err(|e| VulkanError::SemaphoreCreationError(e))?
        };
        semaphores.push(semaphore);
    }

    Ok(semaphores)
}

pub(super) fn create_fences(device: &ash::Device) -> Result<Vec<vk::Fence>, VulkanError> {
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

pub(super) fn create_memory_allocator(
    instance: &ash::Instance,
    device: &ash::Device,
    physical_device: vk::PhysicalDevice,
) -> Result<vk_mem::Allocator, VulkanError> {
    let mut ci = vk_mem::AllocatorCreateInfo::new(instance, device, physical_device);
    ci.flags = vk_mem::AllocatorCreateFlags::BUFFER_DEVICE_ADDRESS;
    Ok(unsafe {
        vk_mem::Allocator::new(ci).map_err(|e| VulkanError::MemoryAllocatorCreationError(e))
    })?
}
