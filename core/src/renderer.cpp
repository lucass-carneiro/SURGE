#include "renderer.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "window.hpp"

#include <algorithm>
#include <array>
#include <cstring>

static void glfw_gl_framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

// See https://www.khronos.org/opengl/wiki/OpenGL_Error#Catching_errors_.28the_easy_way.29
#ifdef SURGE_GL_LOG
static void GLAPIENTRY gl_error_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei,
                                         const GLchar *message, const void *) {

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
#  ifdef SURGE_LOG_GL_NOTIFICATIONS
    log_info("OpenGL info: {}", message);
#  endif
  } else if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_LOW_ARB) {
    log_warn("OpenGL low severity warning: {}", message);
  } else if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_MEDIUM_ARB) {
    log_warn("OpenGL medium severity warning: {}", message);
  } else {
    log_error("OpenGL error: {}", message);
  }
}
#endif

auto surge::renderer::init_opengl(const config::renderer_attrs &r_attrs) noexcept
    -> std::optional<error> {
  /***********************
   * OpenGL context init *
   ***********************/
  log_info("Initializing OpenGL");

  glfwMakeContextCurrent(window::get_window_ptr());
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_make_ctx;
  }

  if (r_attrs.vsync) {
    log_info("VSync enabled");
    glfwSwapInterval(1);
  } else {
    glfwSwapInterval(0);
    log_info("VSync disabled");
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_vsync;
  }

  /********
   * GLAD *
   ********/
  log_info("Initializing GLAD");

  // NOLINTNEXTLINE
  if (gladLoadGL() == 0) {
    log_error("Failed to initialize GLAD");
    glfwTerminate();
    return error::glad_loading;
  }

  // Check extension support
  if (!GLAD_GL_ARB_bindless_texture) {
    log_error("SURGE needs an OpenGL implementation that supports bindless textures and the "
              "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
              "until your graphics card vendor adds this support or it becomes a standardized "
              "OpenGL feature and your vendor produces drivers that support it.");
    glfwTerminate();
    return error::opengl_feature_missing;
  }

  if (!GLAD_GL_ARB_gpu_shader_int64) {
    log_error("SURGE needs an OpenGL implementation that supports int64 in GPU shaders and the "
              "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
              "until your graphics card vendor adds this support or it becomes a standardized "
              "OpenGL feature and your vendor produces drivers that support it.");
    glfwTerminate();
    return error::opengl_feature_missing;
  }

  // Resize callback
  glfwSetFramebufferSizeCallback(window::get_window_ptr(), glfw_gl_framebuffer_size_callback);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_resize_callback;
  }

  /******************
   * OpenGL options *
   ******************/
  // NOLINTNEXTLINE
  log_info("Using OpenGL Version {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

#ifdef SURGE_GL_LOG
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(gl_error_callback, nullptr);
#endif

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // MSAA
  if (r_attrs.MSAA) {
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);
  }

  return {};
}

static auto vk_malloc(void *, size_t size, size_t alignment,
                      [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_alloc(size, alignment);
}

static auto vk_realloc(void *, void *pOriginal, size_t size, size_t alignment,
                       [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_realloc(pOriginal, size, alignment);
}

static void vk_free(void *, void *pMemory) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory free event");
#endif
  surge::allocators::mimalloc::free(pMemory);
}

static void vk_internal_malloc([[maybe_unused]] void *, [[maybe_unused]] size_t size,
                               [[maybe_unused]] VkInternalAllocationType type,
                               [[maybe_unused]] VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc event:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static void vk_internal_free([[maybe_unused]] void *, [[maybe_unused]] size_t size,
                             [[maybe_unused]] VkInternalAllocationType type,
                             [[maybe_unused]] VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc free:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static VkAllocationCallbacks vk_alloc_callbacks{.pUserData = nullptr,
                                                .pfnAllocation = vk_malloc,
                                                .pfnReallocation = vk_realloc,
                                                .pfnFree = vk_free,
                                                .pfnInternalAllocation = vk_internal_malloc,
                                                .pfnInternalFree = vk_internal_free};
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback, void *) {

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
#endif

auto surge::renderer::vk::init(const config::window_resolution &w_res,
                               const config::window_attrs &w_attrs) noexcept
    -> tl::expected<context, error> {
  context ctx{};
  log_info("Initializing Vulkan");

  /*****************************
   * Query required extensions *
   *****************************/
  log_info("Querying required Vulkan instance extensions");

  // GLFW extensions
  u32 glfw_extension_count{0};
  const auto glfw_extensions{glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    return tl::unexpected{error::glfw_vk_ext_retrive};
  }

  vector<const char *> required_extensions{glfw_extensions, glfw_extensions + glfw_extension_count};

  // Debug handler (if validation layers are available)
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  // Printout
  for (const auto &ext : required_extensions) {
    log_info("Vulkan extension requested: {}", ext);
  }

  /************
   * Instance *
   ************/
  log_info("Creating Vulkan instance");

  vkb::InstanceBuilder builder{};
  builder.set_engine_name("SURGE - The Super Underrated Game Engine");
  builder.set_app_name(w_attrs.name.c_str());

  builder.set_app_version(VK_MAKE_API_VERSION(0, 1, 0, 0));
  builder.set_engine_version(
      VK_MAKE_API_VERSION(0, SURGE_VERSION_MAJOR, SURGE_VERSION_MINOR, SURGE_VERSION_PATCH));

  builder.require_api_version(1, 3, 0);
  builder.enable_extensions(required_extensions);

  builder.set_allocation_callbacks(&vk_alloc_callbacks);

  // Message handler and validation layers
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  log_info("Enabling Vulkan validation layers");
  builder.enable_layer("VK_LAYER_KHRONOS_validation");
  builder.request_validation_layers(true);
  builder.set_debug_callback(vk_debug_callback);
#endif

  const auto instance_result{builder.build()};

  if (!instance_result) {
    log_error("Error while initializing Vulkan instance {}",
              string_VkResult(instance_result.vk_result()));
    return tl::unexpected{error::vk_instance_init};
  } else {
    ctx.instance = instance_result.value();
  }

  /**********
   * Device *
   **********/

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // Surface
  const auto surface_result{glfwCreateWindowSurface(ctx.instance.instance, window::get_window_ptr(),
                                                    &vk_alloc_callbacks, &ctx.surface)};
  if (surface_result != VK_SUCCESS) {
    log_error("Window Vulkan surface creation failed: {}", string_VkResult(surface_result));
    return tl::unexpected{error::vk_surface_init};
  }

  // Physical device
  vkb::PhysicalDeviceSelector phys_dev_select{ctx.instance};
  phys_dev_select.set_minimum_version(1, 3);
  phys_dev_select.set_required_features_13(features);
  phys_dev_select.set_required_features_12(features12);
  phys_dev_select.set_surface(ctx.surface);

  const auto phys_dev_select_result{phys_dev_select.select()};

  if (!phys_dev_select_result) {
    log_error("Error while selecting Vulkan physical device {}",
              string_VkResult(phys_dev_select_result.vk_result()));
    return tl::unexpected{error::vk_phys_dev_select};
  } else {
    log_info("Selected Vulkan device {}", phys_dev_select_result.value().name);
    ctx.phys_device = phys_dev_select_result.value();
  }

  // Logical device
  vkb::DeviceBuilder logi_dev_build{phys_dev_select_result.value()};
  const auto device_build_result{logi_dev_build.build()};

  if (!device_build_result) {
    log_error("Error while creating Vulkan logical device {}",
              string_VkResult(device_build_result.vk_result()));
    return tl::unexpected{error::vk_logi_dev_select};
  } else {
    ctx.device = device_build_result.value();
  }

  /*************
   * Swapchain *
   *************/
  const auto swpc_create_result{
      create_swapchain(ctx, static_cast<u32>(w_res.width), static_cast<u32>(w_res.height))};
  if (!swpc_create_result) {
    return tl::unexpected{swpc_create_result.error()};
  } else {
    ctx.swpc_data = swpc_create_result.value();
  }

  return ctx;
}

auto surge::renderer::vk::create_swapchain(context &ctx, u32 width, u32 height) noexcept
    -> tl::expected<swapchain_data, error> {
  swapchain_data swpc_data{};

  vkb::SwapchainBuilder swpc_builder{ctx.phys_device, ctx.device, ctx.surface};

  swpc_builder.set_desired_format(VkSurfaceFormatKHR{
      .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
  swpc_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // TODO: Control VSync here
  swpc_builder.set_desired_extent(width, height);
  swpc_builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  swpc_builder.set_allocation_callbacks(&vk_alloc_callbacks);

  const auto swpc_build_result{swpc_builder.build()};
  if (!swpc_build_result) {
    log_error("Error while creating Vulkan swapchain {}",
              string_VkResult(swpc_build_result.vk_result()));
    return tl::unexpected{error::vk_init_swapchain};
  } else {
    swpc_data.swapchain = swpc_build_result.value();
  }

  const auto get_swpc_img_result{swpc_data.swapchain.get_images()};
  if (!get_swpc_img_result) {
    log_error("Error while retrieving Vulkan swapchain images {}",
              string_VkResult(get_swpc_img_result.vk_result()));
    return tl::unexpected{error::vk_swachain_imgs};
  } else {
    swpc_data.imgs = get_swpc_img_result.value();
  }

  const auto get_swpc_img_views_result{swpc_data.swapchain.get_image_views()};
  if (!get_swpc_img_views_result) {
    log_error("Error while retrieving Vulkan swapchain images {}",
              string_VkResult(get_swpc_img_views_result.vk_result()));
    return tl::unexpected{error::vk_swachain_imgs_views};
  } else {
    swpc_data.imgs_views = get_swpc_img_views_result.value();
  }

  return swpc_data;
}

void surge::renderer::vk::destroy_swapchain(context &ctx, swapchain_data &swpc) noexcept {
  vkb::destroy_swapchain(swpc.swapchain);

  for (auto &img : swpc.imgs_views) {
    vkDestroyImageView(ctx.device.device, img, &vk_alloc_callbacks);
  }

  swpc.imgs.clear();
  swpc.imgs_views.clear();
}

void surge::renderer::vk::terminate(context &ctx) {
  log_info("Terminating Vulkan");

  log_info("Destroying swapchain");
  destroy_swapchain(ctx, ctx.swpc_data);

  log_info("Destroying window surface");
  vkb::destroy_surface(ctx.instance, ctx.surface);

  log_info("Destroying logical device");
  vkb::destroy_device(ctx.device);

  log_info("Destroying instance");
  vkb::destroy_instance(ctx.instance);
}