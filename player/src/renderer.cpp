#include "renderer.hpp"

#include "allocators.hpp"
#include "error_types.hpp"
#include "integer_types.hpp"
#include "logging.hpp"
#include "options.hpp"

#include <cstring>
#include <optional>

static auto vk_alloc(void *, surge::usize size, surge::usize alignment,
                     [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
  auto p{surge::allocators::mimalloc::malloc(size, alignment)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "address: %p\n"
            "failed: %s"
            "scope: %i",
            size, p, p ? "false" : "true", scope);
#endif
  return p;
}

static auto vk_realloc(void *, void *original, surge::usize size, surge::usize alignment,
                       [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
  auto q{surge::allocators::mimalloc::realloc(original, size, alignment)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Memory Event\n"
            "---\n"
            "type: realloc\n"
            "allocator: \"mimalloc::realloc\"\n"
            "new size: %zu\n"
            "old address: %p\n"
            "new address: %p\n"
            "failed: %s"
            "scope: %i",
            size, original, q, q ? "false" : "true", scope);
#endif
  return q;
}

static void vk_free(void *, void *p) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::free\"\n"
            "address: %p",
            p);
#endif
  surge::allocators::mimalloc::free(p);
}

static void vk_internal_alloc(void *, [[maybe_unused]] surge::usize size,
                              [[maybe_unused]] VkInternalAllocationType type,
                              [[maybe_unused]] VkSystemAllocationScope scope) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Internal Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "type: %i\n"
            "scope: %i",
            size, type, scope);
#endif
}

static void vk_internal_free(void *, [[maybe_unused]] surge::usize size,
                             [[maybe_unused]] VkInternalAllocationType type,
                             [[maybe_unused]] VkSystemAllocationScope scope) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Internal Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "type: %i\n"
            "scope: %i",
            size, type, scope);
#endif
}

static auto check_vk_validation_layers(const surge::vector<const char *> &requested_layers) noexcept
    -> bool {
  using namespace surge;
  using std::strcmp;

  u32 layer_count{0};
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  vector<VkLayerProperties> available_layers(layer_count);
  available_layers.reserve(layer_count);

  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  log_info("There are %u validation layers available:", layer_count);
  for (const auto &layer : available_layers) {
    log_info("  Validation layer: %s", layer.layerName);
  }

  for (const auto &requested_layer : requested_layers) {
    bool layer_found = false;

    for (const auto &available_layer : available_layers) {
      if (strcmp(requested_layer, available_layer.layerName) == 0) {
        layer_found = true;
        log_info("Enabeling %s", requested_layer);
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
}

static VKAPI_ATTR auto VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *) noexcept -> VkBool32 {

  switch (severity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    log_info("Vulkan Diagnostic: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    log_info("Vulkan Info: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    log_warn("Vulkan Warning: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    log_error("Vulkan Error: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    break;
  }

  return VK_FALSE;
}

auto get_extensions() noexcept -> tl::expected<surge::vector<const char *>, surge::error> {
  using namespace surge;
  u32 glfw_ext_count{0};

  auto glfw_ext_list{glfwGetRequiredInstanceExtensions(&glfw_ext_count)};
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_error("  Ubable to query required Vulkan Extensions from GLFW.");
    return tl::unexpected{surge::error::glfw_ext_querry};
  }

  vector<const char *> extensions(glfw_ext_list, glfw_ext_list + glfw_ext_count);

#ifdef SURGE_BUILD_TYPE_Debug
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

auto create_debug_msg(surge::renderer::context &vk_ctx,
                      VkDebugUtilsMessengerCreateInfoEXT &dmci) noexcept
    -> std::optional<surge::error> {
  using namespace surge;

  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(vk_ctx.instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    if (func(vk_ctx.instance, &dmci, &vk_ctx.alloc_callbacks, &vk_ctx.debug_messager)
        != VK_SUCCESS) {
      log_error("Error while creating Debug Messenger.");
      return error::vk_debug_msg;
    }
  } else {
    log_error("vkCreateDebugUtilsMessengerEXT not present");
    return error::vk_ext_not_found;
  }

  return {};
}

void destroy_debug_msg(surge::renderer::context &vk_ctx) noexcept {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(vk_ctx.instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(vk_ctx.instance, vk_ctx.debug_messager, &vk_ctx.alloc_callbacks);
  }
}

auto surge::renderer::init(const string &window_name) noexcept -> tl::expected<context, error> {
  log_info("Initializing Vulkan");

  context vk_ctx{};

  /**************
   * Extensions *
   **************/
  auto extensions{get_extensions()};
  if (!extensions) {
    return tl::unexpected{extensions.error()};
  }

  /************
   * App info *
   ************/
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = window_name.c_str();
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "SURGE - The Super Underrated Game Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  /***************
   * Create Info *
   ***************/
  VkInstanceCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &app_info;
  ici.enabledExtensionCount = static_cast<surge::u32>(extensions->size());
  ici.ppEnabledExtensionNames = extensions->data();

  /*************************
   * Debug MSG create info *
   *************************/
  VkDebugUtilsMessengerCreateInfoEXT dmci{};
  dmci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  dmci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dmci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  dmci.pfnUserCallback = debug_callback;
  dmci.pUserData = nullptr;

  /**************
   * Allocators *
   **************/
  vk_ctx.alloc_callbacks
      = {nullptr, vk_alloc, vk_realloc, vk_free, vk_internal_alloc, vk_internal_free};

/*********************
 * Validation Layers *
 *********************/
#ifdef SURGE_BUILD_TYPE_Debug
  const vector<const char *> requested_layers = {"VK_LAYER_KHRONOS_validation"};

  if (!check_vk_validation_layers(requested_layers)) {
    log_error("Requested validation layers not available");
    return tl::unexpected{error::vk_validation_layers_not_available};
  }

  ici.enabledLayerCount = static_cast<surge::u32>(requested_layers.size());
  ici.ppEnabledLayerNames = requested_layers.data();
  ici.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&dmci);
#else
  ici.enabledLayerCount = 0;
#endif

  /************
   * Instance *
   ************/
  if (vkCreateInstance(&ici, &vk_ctx.alloc_callbacks, &vk_ctx.instance) != VK_SUCCESS) {
    log_error("Error while creating vulkan instance");
    return tl::unexpected{error::vk_instance_create};
  }

  /******************
   * Debug Messager *
   ******************/
  auto create_debug_msg_result{create_debug_msg(vk_ctx, dmci)};
  if (create_debug_msg_result.has_value()) {
    return tl::unexpected{create_debug_msg_result.value()};
  }

  log_info("Vulkan initialized");

  return vk_ctx;
}

void surge::renderer::terminate(context &vk_ctx) noexcept {
  log_info("Terminating Vulkan");

  /******************
   * Debug Messager *
   ******************/
  destroy_debug_msg(vk_ctx);

  /************
   * Instance *
   ************/
  vkDestroyInstance(vk_ctx.instance, &vk_ctx.alloc_callbacks);
}