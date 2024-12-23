#include "sc_vulkan/atoms/shader.hpp"

#include "../sc_vulkan_malloc.hpp"
#include "../sc_vulkan_types.hpp"
#include "sc_files.hpp"
#include "sc_logging.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::vk_atom::shader::load_shader_module(renderer::vk::context ctx,
                                                const char *path) noexcept
    -> tl::expected<VkShaderModule, error> {
  log_info("Loading vulkan shader module {}", path);

  const auto file{files::load_file(path, false)};

  if (!file) {
    log_error("Unable to load Vulkan shader file {}", path);
    return tl::unexpected{file.error()};
  }

  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.pNext = nullptr;
  info.codeSize = file->size();
  info.pCode = reinterpret_cast<const u32 *>(file->data()); // NOLINT

  VkShaderModule module{};
  const auto result{
      vkCreateShaderModule(ctx->device, &info, renderer::vk::get_alloc_callbacks(), &module)};

  if (result != VK_SUCCESS) {
    log_error("Unable create shader module from file {}: {}", path, string_VkResult(result));
    return tl::unexpected{error::vk_shader_module_create};
  } else {
    log_info("Loaded shader module {}, handle {}", path, static_cast<void *>(module));
    return module;
  }
}

void surge::vk_atom::shader::destroy_shader_module(renderer::vk::context ctx,
                                                   VkShaderModule module) noexcept {
  log_info("Unloading shader module handle {}", static_cast<void *>(module));
  vkDestroyShaderModule(ctx->device, module, renderer::vk::get_alloc_callbacks());
}

auto surge::vk_atom::shader::create_shader_object(renderer::vk::context ctx,
                                                  const char *vertex_shader_path,
                                                  const char *fragment_shader_path) noexcept
    -> tl::expected<program_shaders, error> {
  log_info("Creating shader objects from {} and {}", vertex_shader_path, fragment_shader_path);

  // Find function
  auto func{reinterpret_cast<PFN_vkCreateShadersEXT>(
      vkGetInstanceProcAddr(ctx->instance, "vkCreateShadersEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkCreateShadersEXT");
    return tl::unexpected{error::vk_shader_obj_ext_func_ptr};
  }

  // Load files
  const auto vs_file{files::load_file(vertex_shader_path, false)};
  const auto fs_file{files::load_file(fragment_shader_path, false)};

  if (!vs_file) {
    log_error("Unable to load Vulkan vertex shader file {}", vertex_shader_path);
    return tl::unexpected{vs_file.error()};
  }

  if (!fs_file) {
    log_error("Unable to load Vulkan fragment shader file {}", fragment_shader_path);
    return tl::unexpected{fs_file.error()};
  }

  // Common options
  const VkShaderCreateFlagsEXT flags{VK_SHADER_CREATE_LINK_STAGE_BIT_EXT};
  const VkShaderCodeTypeEXT code_type{VK_SHADER_CODE_TYPE_SPIRV_EXT};
  const char *entry_point{"main"};

  // VS info
  VkShaderCreateInfoEXT vs_info{};
  vs_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  vs_info.pNext = nullptr;
  vs_info.flags = flags;
  vs_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vs_info.nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
  vs_info.codeType = code_type;
  vs_info.codeSize = vs_file->size();
  vs_info.pCode = vs_file->data();
  vs_info.pName = entry_point;

  // FS info
  VkShaderCreateInfoEXT fs_info{};
  fs_info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  fs_info.pNext = nullptr;
  fs_info.flags = flags;
  fs_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fs_info.codeType = code_type;
  fs_info.codeSize = fs_file->size();
  fs_info.pCode = fs_file->data();
  fs_info.pName = entry_point;

  // Shader creation
  const std::array<VkShaderCreateInfoEXT, 2> shader_infos{vs_info, fs_info};

  program_shaders shaders{};
  const auto result{func(ctx->device, 2, shader_infos.data(), renderer::vk::get_alloc_callbacks(),
                         shaders.data())};

  if (result != VK_SUCCESS) {
    log_error("Unable create shader objects from files {} and {}: {}", vertex_shader_path,
              fragment_shader_path, string_VkResult(result));
    return tl::unexpected{error::vk_shader_object_create};
  } else {
    log_info("Created shader objects from {} and {}", vertex_shader_path, fragment_shader_path);
    return shaders;
  }
}

void surge::vk_atom::shader::destroy_shader_object(renderer::vk::context ctx,
                                                   program_shaders &shaders) noexcept {
  log_info("Destroying shader object");

  // Find function
  auto func{reinterpret_cast<PFN_vkDestroyShaderEXT>(
      vkGetInstanceProcAddr(ctx->instance, "vkDestroyShaderEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkDestroyShaderEXT");
  } else {
    for (auto &shader : shaders) {
      func(ctx->device, shader, renderer::vk::get_alloc_callbacks());
    }
  }
}