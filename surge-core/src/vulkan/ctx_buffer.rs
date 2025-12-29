use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk;
use vk_mem::{self, Alloc};

pub struct Buffer {
    pub buffer: vk::Buffer,
    pub allocation: vk_mem::Allocation,
}

impl VulkanContext {
    pub fn create_buffer(
        &self,
        ci: &vk::BufferCreateInfo,
        ai: &vk_mem::AllocationCreateInfo,
    ) -> Result<Buffer, VulkanError> {
        let (buffer, allocation) = unsafe {
            self.memory_allocator
                .create_buffer(ci, ai)
                .map_err(|e| VulkanError::BufferAllocationError(e))
        }?;

        Ok(Buffer { buffer, allocation })
    }

    pub fn destroy_buffer(&self, buffer: &mut Buffer) {
        unsafe {
            self.memory_allocator
                .destroy_buffer(buffer.buffer, &mut buffer.allocation)
        };
    }
}
