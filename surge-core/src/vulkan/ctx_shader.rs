use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk::{self, Handle};
use log;
use std::mem::size_of;

impl VulkanContext {
    pub fn load_shader_module(&self, spirv_bytes: &[u8]) -> Result<vk::ShaderModule, VulkanError> {
        log::info!("Creating SPIRV shader module ({} bytes)", spirv_bytes.len());

        // Vulkan wants shaders to be u32 buffers.
        // Check if the byte slice size is a multiple of sizeof(u32)
        let byte_size = spirv_bytes.len();
        let u32_size = size_of::<u32>();

        if byte_size % u32_size != 0 {
            return Err(VulkanError::ShaderModuleInvalidSizeError(byte_size));
        }

        // Reinterpret the byte slice as a u32 vector.
        // The key is to use the `from_ne_bytes` method (native endianness) or
        // explicitly specify the endianness (`from_le_bytes` or `from_be_bytes`)
        // based on how the bytes were originally written.
        let u32_buffer: Vec<u32> = spirv_bytes
            .chunks_exact(u32_size)
            .map(|bytes_slice| {
                let bytes_array: [u8; 4] = bytes_slice.try_into().unwrap();
                u32::from_ne_bytes(bytes_array)
            })
            .collect();

        let ci = vk::ShaderModuleCreateInfo {
            code_size: byte_size,
            p_code: u32_buffer.as_ptr(),
            ..Default::default()
        };

        let sm = unsafe {
            self.device
                .create_shader_module(&ci, None)
                .map_err(|e| VulkanError::ShaderModuleCreateError(e))
        }?;

        log::info!("Created shader module {:#x}", sm.as_raw());
        Ok(sm)
    }

    pub fn unload_shader_module(&self, sm: vk::ShaderModule) {
        log::info!("Unloading shader module {:#x}", sm.as_raw());
        unsafe {
            self.device.destroy_shader_module(sm, None);
        }
    }
}
