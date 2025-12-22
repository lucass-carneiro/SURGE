use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk::{self, Handle};
use log;
use std::{fs::File, io::Read, mem::size_of};

impl VulkanContext {
    pub fn load_shader_module(&self, spirv_path: &str) -> Result<vk::ShaderModule, VulkanError> {
        log::info!("Loading {} as SPIRV shader module", spirv_path);

        // Read file into buffer
        let mut file = File::open(spirv_path).map_err(|e| VulkanError::ShaderModuleReadError(e))?;
        let mut byte_buffer = Vec::new();
        file.read_to_end(&mut byte_buffer)
            .map_err(|e| VulkanError::ShaderModuleReadError(e))?;

        // Vulkan wants shaders to be u32 buffers.
        // Check if the file size is a multiple of sizeof(u32)
        let file_size = byte_buffer.len();
        let u32_size = size_of::<u32>();

        if file_size % u32_size != 0 {
            return Err(VulkanError::ShaderModuleInvalidSizeError(
                spirv_path.to_string(),
                file_size,
            ));
        }

        // Reinterpret the byte vector as a u32 vector.
        // The key is to use the `from_ne_bytes` method (native endianness) or
        // explicitly specify the endianness (`from_le_bytes` or `from_be_bytes`)
        // based on how the file was originally written.
        let u32_buffer: Vec<u32> = byte_buffer
            .chunks_exact(u32_size)
            .map(|bytes_slice| {
                let bytes_array: [u8; 4] = bytes_slice.try_into().unwrap();
                u32::from_ne_bytes(bytes_array)
            })
            .collect();

        let ci = vk::ShaderModuleCreateInfo {
            code_size: file_size,
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
