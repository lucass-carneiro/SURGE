use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk::{self, Handle};
use log;
use std::mem::size_of;

impl VulkanContext {
    pub fn load_shader_module(&self, spirv_bytes: &[u8]) -> Result<vk::ShaderModule, VulkanError> {
        log::info!("Creating SPIRV shader module ({} bytes)", spirv_bytes.len());

        let u32_buffer = spirv_words(spirv_bytes)?;

        let ci = vk::ShaderModuleCreateInfo {
            code_size: spirv_bytes.len(),
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

/// The first word of every SPIR-V module, in the module's own endianness.
const SPIRV_MAGIC: u32 = 0x0723_0203;

/// Copies SPIR-V bytes into a `Vec<u32>` and checks the magic number.
///
/// The copy is not incidental: `p_code` must be 4-byte aligned, which a `&[u8]` (such as the
/// `include_bytes!` data) does not guarantee, and a `Vec<u32>` does. Words are read in native
/// endianness, which is what Vulkan requires; a byte-swapped module fails the magic check.
fn spirv_words(spirv_bytes: &[u8]) -> Result<Vec<u32>, VulkanError> {
    let byte_size = spirv_bytes.len();

    if byte_size % size_of::<u32>() != 0 {
        return Err(VulkanError::ShaderModuleInvalidSizeError(byte_size));
    }

    let words: Vec<u32> = spirv_bytes
        .chunks_exact(size_of::<u32>())
        .map(|chunk| u32::from_ne_bytes(chunk.try_into().unwrap()))
        .collect();

    if words.first() != Some(&SPIRV_MAGIC) {
        return Err(VulkanError::ShaderModuleInvalidMagicError);
    }

    Ok(words)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn spirv_words_accepts_a_module_with_the_magic_number() {
        let mut bytes = SPIRV_MAGIC.to_ne_bytes().to_vec();
        bytes.extend_from_slice(&1u32.to_ne_bytes());
        assert_eq!(spirv_words(&bytes).unwrap(), vec![SPIRV_MAGIC, 1]);
    }

    #[test]
    fn spirv_words_rejects_bad_size_magic_and_empty_input() {
        assert!(matches!(
            spirv_words(&[1, 2, 3]),
            Err(VulkanError::ShaderModuleInvalidSizeError(3))
        ));
        assert!(matches!(
            spirv_words(&SPIRV_MAGIC.swap_bytes().to_ne_bytes()),
            Err(VulkanError::ShaderModuleInvalidMagicError)
        ));
        assert!(matches!(
            spirv_words(&[]),
            Err(VulkanError::ShaderModuleInvalidMagicError)
        ));
    }
}
