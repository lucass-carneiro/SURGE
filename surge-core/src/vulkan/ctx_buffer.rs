use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk;
use vk_mem::{self, Alloc};

pub struct Buffer {
    buffer: vk::Buffer,
    allocation: vk_mem::Allocation,
}

impl VulkanContext {
    pub fn create_buffer(
        &self,
        size: u64,
        usage_flags: vk::BufferUsageFlags,
        memory_usage: vk_mem::MemoryUsage,
    ) -> Result<Buffer, VulkanError> {
        let buffer_info = vk::BufferCreateInfo {
            size: size,
            usage: usage_flags,
            ..Default::default()
        };

        let create_info = vk_mem::AllocationCreateInfo {
            usage: memory_usage,
            flags: vk_mem::AllocationCreateFlags::MAPPED,
            ..Default::default()
        };

        let (buffer, allocation) = unsafe {
            self.memory_allocator
                .create_buffer(&buffer_info, &create_info)
                .map_err(|e| VulkanError::BufferAllocationError(e))
        }?;

        Ok(Buffer { buffer, allocation })
    }

    pub fn destroy_buffer(&self, mut buffer: Buffer) {
        unsafe {
            self.memory_allocator
                .destroy_buffer(buffer.buffer, &mut buffer.allocation)
        };
    }
}
