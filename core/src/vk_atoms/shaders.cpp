#include "vk_atoms/shaders.hpp"

#include "files.hpp"
#include "logging.hpp"
#include "renderer_vk_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::vk_atom::shader::load_shader_module(VkDevice device, const char *path) noexcept
    -> tl::expected<VkShaderModule, error> {
  log_info("Loading vulkan shader module {}", path);

  const auto file{files::load_file(path, false)};

  if (!file) {
    log_error("Unable to load vulkan shader file {}", path);
    return tl::unexpected{file.error()};
  }

  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.pNext = nullptr;
  info.codeSize = file->size();
  info.pCode = reinterpret_cast<const u32 *>(file->data()); // NOLINT

  VkShaderModule module{};
  const auto result{
      vkCreateShaderModule(device, &info, renderer::vk::get_alloc_callbacks(), &module)};

  if (result != VK_SUCCESS) {
    log_error("Unable create shader module from file {}: {}", path, string_VkResult(result));
    return tl::unexpected{error::vk_shader_module_create};
  } else {
    log_info("Loaded shader module {}, handle {}", path, static_cast<void *>(module));
    return module;
  }
}

void surge::vk_atom::shader::destroy_shader_module(VkDevice device,
                                                   VkShaderModule module) noexcept {
  log_info("Unloading shader module handle {}", static_cast<void *>(module));
  vkDestroyShaderModule(device, module, renderer::vk::get_alloc_callbacks());
}