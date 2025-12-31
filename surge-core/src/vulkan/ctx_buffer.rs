use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk;
use vk_mem::{self, Alloc};

pub struct Buffer {
    pub buffer: vk::Buffer,
    pub allocation: vk_mem::Allocation,
    pub allocation_info: vk_mem::AllocationInfo,
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

        let allocation_info = self.memory_allocator.get_allocation_info(&allocation);

        Ok(Buffer {
            buffer,
            allocation,
            allocation_info,
        })
    }

    pub fn destroy_buffer(&self, buffer: &mut Buffer) {
        unsafe {
            self.memory_allocator
                .destroy_buffer(buffer.buffer, &mut buffer.allocation)
        };
    }
}
