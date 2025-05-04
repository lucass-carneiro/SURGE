#include "sc_vulkan/sc_vulkan_pipelines.hpp"

#include "sc_files.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::load_shader_module(Context ctx,
                                             const char *spirv_path) -> Result<VkShaderModule> {

  log_info("Loading {} as SPIRV shader module", spirv_path);

  const auto file_data{files::as_bytes(spirv_path, false)};
  if (!file_data) {
    log_error("Unable to read SPIRV file {}", spirv_path);
    return Err{file_data.error()};
  }

  // Vulkan wants the spirv data to be a u32 buffer
  const auto spirv_data{reinterpret_cast<const u32 *>(file_data->data())};

  // create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = nullptr;
  create_info.codeSize = file_data->size();
  create_info.pCode = spirv_data;

  VkShaderModule shader_module{};
  const auto result{
      vkCreateShaderModule(ctx->device, &create_info, get_alloc_callbacks(), &shader_module)};

  if (result != VK_SUCCESS) {
    log_error("Unable create shader module from SPIRV file {}: {}", spirv_path,
              string_VkResult(result));
    return Err{Error::vk_shader_module_create};
  } else {
    log_info("Loaded {} as SPIRV shader module, handle {}", spirv_path,
             static_cast<void *>(shader_module));
    return shader_module;
  }
}

void surge::renderer::vk::destroy_shader_module(Context ctx, VkShaderModule shader_module) {
  log_info("Destroying SPIRV shader module handle {}", static_cast<void *>(shader_module));
  vkDestroyShaderModule(ctx->device, shader_module, get_alloc_callbacks());
}