#include "renderer.hpp"

#include "allocators.hpp"
#include "error_types.hpp"
#include "integer_types.hpp"
#include "logging.hpp"
#include "options.hpp"

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

// NOLINTNEXTLINE
static VkAllocationCallbacks vk_alloc_callbacks{nullptr, vk_alloc,          vk_realloc,
                                                vk_free, vk_internal_alloc, vk_internal_free};

auto surge::renderer::init(const string &window_name, GLFWwindow *window) noexcept
    -> tl::expected<context, error> {
  log_info("Initializing Vulkan");

  /************
   * Instance *
   ************/
  vkb::InstanceBuilder ib{};

#ifdef SURGE_BUILD_TYPE_Debug
  ib.request_validation_layers(true);
#else
  ib.request_validation_layers(false);
#endif

  ib.set_debug_callback(debug_callback);
  ib.set_allocation_callbacks(&vk_alloc_callbacks);
  ib.set_app_name(window_name.c_str());
  ib.set_engine_name("SURGE - The Super Underrated Game Engine");
  ib.require_api_version(1, 3, 0);

  auto ib_result{ib.build()};
  if (!ib_result) {
    log_error("Unable to create Vulkan instance: %s", ib_result.error().message().c_str());
    return tl::unexpected{error::vk_instance_creation};
  }

  /************
   * Device *
   ************/
  VkSurfaceKHR surface{};
  if (glfwCreateWindowSurface(ib_result->instance, window, &vk_alloc_callbacks, &surface)
      != VK_SUCCESS) {
    log_error("Unable to acquire Vulkan surface");
    return tl::unexpected{error::vk_surface};
  }

  /************
   * Device *
   ************/
  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  vkb::PhysicalDeviceSelector pds{ib_result.value()};
  pds.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
  pds.set_minimum_version(1, 3);
  pds.set_required_features_13(features);
  pds.set_required_features_12(features12);
  pds.set_surface(surface);

  auto pds_result{pds.select()};

  if (!pds_result) {
    log_error("Unable to select physical device: %s", pds_result.error().message().c_str());
    return tl::unexpected{error::vk_physical_device};
  }

  vkb::DeviceBuilder db{pds_result.value()};
  db.set_allocation_callbacks(&vk_alloc_callbacks);

  auto db_result{db.build()};
  if (!db_result) {
    log_error("Unable to build logical device: %s", db_result.error().message().c_str());
    return tl::unexpected{error::vk_logical_device};
  }

  /*************
   * Swapchain *
   *************/
  log_info("Creating swapchain");

  const auto sc_img_format{VK_FORMAT_B8G8R8A8_UNORM};
  int ww{0}, wh{0};
  glfwGetWindowSize(window, &ww, &wh);

  vkb::SwapchainBuilder scb{pds_result.value().physical_device, db_result.value().device, surface};
  scb.set_desired_format(
      VkSurfaceFormatKHR{.format = sc_img_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
  scb.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
  scb.set_desired_extent(static_cast<u32>(ww), static_cast<u32>(wh));
  scb.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  scb.set_allocation_callbacks(&vk_alloc_callbacks);

  auto scb_result{scb.build()};
  if (!scb_result) {
    log_error("Unable to build swapchain: %s", scb_result.error().message().c_str());
    return tl::unexpected{error::vk_swapchain_error};
  }

  auto sc_imgs_result{scb_result.value().get_images()};
  if (!sc_imgs_result) {
    log_error("Unable to retrieve swapchain images: %s", sc_imgs_result.error().message().c_str());
    return tl::unexpected{error::vk_swapchain_images};
  }

  auto sc_img_views_result{scb_result.value().get_images()};
  if (!sc_img_views_result) {
    log_error("Unable to retrieve swapchain image views: %s",
              sc_img_views_result.error().message().c_str());
    return tl::unexpected{error::vk_swapchain_image_views};
  }

  log_info("Vulkan initialized");

  return context{ib_result.value(), db_result.value(), surface, scb_result.value()};
}

void surge::renderer::terminate(context &ctx) noexcept {
  log_info("Terminating Vulkan");

  vkb::destroy_swapchain(ctx.swapchain);
  vkDestroySurfaceKHR(ctx.instance.instance, ctx.surface, &vk_alloc_callbacks);
  vkb::destroy_device(ctx.device);
  vkb::destroy_instance(ctx.instance);
}