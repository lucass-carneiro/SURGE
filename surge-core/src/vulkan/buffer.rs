use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk;
use std::{cell::RefCell, sync::Arc};
use vk_mem::{self, Alloc};

pub struct Buffer {
    context: Arc<RefCell<VulkanContext>>,
    buffer: vk::Buffer,
    allocation: vk_mem::Allocation,
    allocation_info: vk_mem::AllocationInfo,
}

impl Buffer {
    pub fn new(
        context: Arc<RefCell<VulkanContext>>,
        ci: &vk::BufferCreateInfo,
        ai: &vk_mem::AllocationCreateInfo,
    ) -> Result<Self, VulkanError> {
        let (buffer, allocation) = unsafe {
            context
                .borrow()
                .memory_allocator
                .create_buffer(ci, ai)
                .map_err(|e| VulkanError::BufferAllocationError(e))
        }?;

        let allocation_info = context
            .borrow()
            .memory_allocator
            .get_allocation_info(&allocation);

        Ok(Buffer {
            context,
            buffer,
            allocation,
            allocation_info,
        })
    }

    pub fn get_buffer(&self) -> vk::Buffer {
        self.buffer
    }

    pub fn get_allocation(&self) -> vk_mem::Allocation {
        self.allocation
    }

    pub fn get_allocation_info(&self) -> &vk_mem::AllocationInfo {
        &self.allocation_info
    }
}

impl Drop for Buffer {
    fn drop(&mut self) {
        unsafe {
            self.context
                .borrow()
                .memory_allocator
                .destroy_buffer(self.buffer, &mut self.allocation)
        };
    }
}
