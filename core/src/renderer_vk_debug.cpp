#include "renderer_vk_debug.hpp"

#include "logging.hpp"
#include "renderer_vk_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

#ifdef SURGE_USE_VK_VALIDATION_LAYERS

static VKAPI_ATTR auto VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback, void *) -> VkBool32 {

  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
      || severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {

    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      log_info("Vulkan Info (General): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      log_info("Vulkan Info (Validation): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      log_info("Vulkan Info (Performance): {}", callback->pMessage);
      break;
    default:
      break;
    }
    return VK_FALSE;

  } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {

    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      log_warn("Vulkan Warning (General): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      log_warn("Vulkan Warning (Validation): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      log_warn("Vulkan Warning (Performance): {}", callback->pMessage);
      break;
    default:
      break;
    }

    return VK_FALSE;

  } else {

    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      log_error("Vulkan Error (General): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      log_error("Vulkan Error (Validation): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      log_error("Vulkan Error (Performance): {}", callback->pMessage);
      break;
    default:
      break;
    }

    return VK_TRUE;
  }
}

auto surge::renderer::vk::dbg_msg_create_info() noexcept -> VkDebugUtilsMessengerCreateInfoEXT {
  VkDebugUtilsMessengerCreateInfoEXT create_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback,
      .pUserData = nullptr};

  return create_info;
}

auto surge::renderer::vk::create_dbg_msg(VkInstance instance) noexcept
    -> tl::expected<VkDebugUtilsMessengerEXT, error> {
  log_info("Creating debug messenger");

  auto func{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkCreateDebugUtilsMessengerEXT");
    return tl::unexpected{error::vk_dbg_msg_ext_func_ptr};
  }

  auto create_info{dbg_msg_create_info()};

  VkDebugUtilsMessengerEXT dbg_msg{};
  const auto result{func(instance, &create_info, get_alloc_callbacks(), &dbg_msg)};

  if (result != VK_SUCCESS) {
    log_error("Unable create debug messenger: {}", string_VkResult(result));
    return tl::unexpected{error::vk_dbg_msg_create};
  } else {
    return dbg_msg;
  }
}

auto surge::renderer::vk::create_dbg_msg(VkInstance instance,
                                         VkDebugUtilsMessengerCreateInfoEXT create_info) noexcept
    -> tl::expected<VkDebugUtilsMessengerEXT, error> {
  log_info("Creating debug messenger");

  auto func{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkCreateDebugUtilsMessengerEXT");
    return tl::unexpected{error::vk_dbg_msg_ext_func_ptr};
  }

  VkDebugUtilsMessengerEXT dbg_msg{};
  const auto result{func(instance, &create_info, get_alloc_callbacks(), &dbg_msg)};

  if (result != VK_SUCCESS) {
    log_error("Unable create debug messenger: {}", string_VkResult(result));
    return tl::unexpected{error::vk_dbg_msg_create};
  } else {
    log_info("Debug messenger created");
    return dbg_msg;
  }
}

auto surge::renderer::vk::destroy_dbg_msg(
    VkInstance instance, VkDebugUtilsMessengerEXT dbg_msg) noexcept -> tl::expected<void, error> {
  log_info("Destroying debug messenger");

  auto func{reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkDestroyDebugUtilsMessengerEXT");
    return tl::unexpected{error::vk_dbg_msg_ext_func_ptr};
  }

  func(instance, dbg_msg, get_alloc_callbacks());

  log_info("Debug messenger destroyed");

  return {};
}

#endif
