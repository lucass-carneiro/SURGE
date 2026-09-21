use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk;
use std::{cell::RefCell, sync::Arc};
use vk_mem::{self, Alloc};

pub struct Buffer {
    context: Arc<RefCell<VulkanContext>>,
    buffer: vk::Buffer,
    allocation: vk_mem::Allocation,
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

        let buf = Buffer {
            context,
            buffer,
            allocation,
        };

        // A buffer that was asked to be persistently mapped must actually be; failing here (the
        // `Drop` of `buf` frees the allocation) beats a null dereference at the first write.
        if ai.flags.contains(vk_mem::AllocationCreateFlags::MAPPED)
            && buf.mapped_ptr::<u8>().is_null()
        {
            return Err(VulkanError::BufferNotMapped);
        }

        Ok(buf)
    }

    pub fn get_buffer(&self) -> vk::Buffer {
        self.buffer
    }

    /// The persistently mapped host pointer, or null if the buffer is not mapped. Queried from
    /// VMA on every call rather than cached, since VMA may move or remap an allocation.
    pub fn mapped_ptr<T>(&self) -> *mut T {
        self.context
            .borrow()
            .memory_allocator
            .get_allocation_info(&self.allocation)
            .mapped_data as *mut T
    }

    /// Flushes the whole allocation's mapped memory range, making CPU writes visible to the
    /// GPU. This is a no-op on `HOST_COHERENT` memory but essential otherwise, and VMA does
    /// not guarantee coherent memory was actually obtained even when requested.
    pub fn flush(&self) -> Result<(), VulkanError> {
        self.context
            .borrow()
            .memory_allocator
            .flush_allocation(&self.allocation, 0, vk::WHOLE_SIZE)
            .map_err(|e| VulkanError::BufferFlushError(e))
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
