#include "renderer.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "window.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <vulkan/vk_enum_string_helper.h>

static void glfw_gl_framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

// See https://www.khronos.org/opengl/wiki/OpenGL_Error#Catching_errors_.28the_easy_way.29
static void GLAPIENTRY gl_error_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei,
                                         const GLchar *message, const void *) {

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
#ifdef SURGE_LOG_GL_NOTIFICATIONS
    log_info("OpenGL info: {}", message);
#endif
  } else if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_LOW_ARB) {
    log_warn("OpenGL low severity warning: {}", message);
  } else if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_MEDIUM_ARB) {
    log_warn("OpenGL medium severity warning: {}", message);
  } else {
    log_error("OpenGL error: {}", message);
  }
}

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

#ifdef SURGE_BUILD_TYPE_Debug
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

static auto vk_malloc(void *, size_t size, size_t alignment, VkSystemAllocationScope scope) noexcept
    -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_alloc(size, alignment);
}

static auto vk_realloc(void *, void *pOriginal, size_t size, size_t alignment,
                       VkSystemAllocationScope scope) noexcept -> void * {
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

static void vk_internal_malloc(void *, size_t size, VkInternalAllocationType type,
                               VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc event:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static void vk_internal_free(void *, size_t size, VkInternalAllocationType type,
                             VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc free:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static const VkAllocationCallbacks vk_alloc_callbacks{.pUserData = nullptr,
                                                      .pfnAllocation = vk_malloc,
                                                      .pfnReallocation = vk_realloc,
                                                      .pfnFree = vk_free,
                                                      .pfnInternalAllocation = vk_internal_malloc,
                                                      .pfnInternalFree = vk_internal_free};
#ifdef SURGE_BUILD_TYPE_Debug
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
    }

    return VK_TRUE;
  }
}
#endif

auto surge::renderer::vk::init(const config::window_attrs &w_attrs) noexcept
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
#ifdef SURGE_BUILD_TYPE_Debug
  required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  // Printout
  for (const auto &ext : required_extensions) {
    log_info("Vulkan extension requested: {}", ext);
  }

  /*********************
   * Validation Layers *
   *********************/
#ifdef SURGE_BUILD_TYPE_Debug
  log_info("Enabling Vulkan validation layers");

  const std::array<const char *, 1> required_validation_layers{"VK_LAYER_KHRONOS_validation"};

  for (const auto &l : required_validation_layers) {
    log_info("Vulkan validation layer requested: {}", l);
  }

  u32 layer_count{0};
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  vector<VkLayerProperties> available_validation_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_validation_layers.data());

  if (available_validation_layers.size() < required_validation_layers.size()) {
    log_error("{} validation layers were requested but only {} are available",
              required_validation_layers.size(), available_validation_layers.size());
    return tl::unexpected{vk_validation_layer_count_incompatible};
  } else {
    for (const auto &l : available_validation_layers) {
      log_info("Vulkan validation layer available: {}", l.layerName);
    }
  }

  for (const auto &layer : required_validation_layers) {
    if (std::none_of(available_validation_layers.begin(), available_validation_layers.end(),
                     [&](auto &p) { return strcmp(layer, p.layerName) == 0; })) {
      log_error("Required validation layer {} not found", layer);
      return tl::unexpected{vk_validation_layer_not_available};
    }
  }
#endif // SURGE_BUILD_TYPE_Debug

  /************
   * Instance *
   ************/
  log_info("Creating Vulkan instance");

  // app info
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = w_attrs.name.c_str();
  app_info.pEngineName = "SURGE - The Super Underrated Game Engine";
  app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  app_info.engineVersion
      = VK_MAKE_API_VERSION(0, SURGE_VERSION_MAJOR, SURGE_VERSION_MINOR, SURGE_VERSION_PATCH);
  app_info.apiVersion = VK_API_VERSION_1_3;

  // Message handler
#ifdef SURGE_BUILD_TYPE_Debug
  VkDebugUtilsMessengerCreateInfoEXT debug_handler_create_info{};
  debug_handler_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_handler_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_handler_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                          | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                          | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_handler_create_info.pfnUserCallback = vk_debug_callback;
  debug_handler_create_info.pUserData = nullptr;
#endif

  // create info
  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = static_cast<u32>(required_extensions.size());
  create_info.ppEnabledExtensionNames = required_extensions.data();

#ifdef SURGE_BUILD_TYPE_Debug
  create_info.enabledLayerCount = static_cast<u32>(required_validation_layers.size());
  create_info.ppEnabledLayerNames = required_validation_layers.data();
  create_info.pNext = &debug_handler_create_info;
#else
  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = nullptr;
  create_info.pNext = nullptr;
#endif

  auto result{vkCreateInstance(&create_info, &vk_alloc_callbacks, &ctx.instance)};

  if (result != VK_SUCCESS) {
    log_error("Error while initializing Vulkan instance {}", string_VkResult(result));
    return tl::unexpected{error::vk_instance_init};
  }

  /*****************
   * Debug handler *
   *****************/
#ifdef SURGE_BUILD_TYPE_Debug
  log_info("Creating debug messenger");

  const auto func{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(ctx.instance, "vkCreateDebugUtilsMessengerEXT"))};

  if (func != nullptr) {
    const auto result{
        func(ctx.instance, &debug_handler_create_info, &vk_alloc_callbacks, &ctx.debug_messenger)};
    if (result != VK_SUCCESS) {
      log_error("Error while creating debug handler: {}", string_VkResult(result));
      return tl::unexpected{error::vk_debug_handler_creation};
    }
  } else {
    log_error("Extension \"vkCreateDebugUtilsMessengerEXT\" is not present.");
    return tl::unexpected{error::vk_debug_handler_creation};
  }
#endif

  return ctx;
}

void surge::renderer::vk::terminate(context &ctx) {
  log_info("Terminating Vulkan");

#ifdef SURGE_BUILD_TYPE_Debug
  log_info("Destroying debug messenger");
  const auto func{reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(ctx.instance, "vkDestroyDebugUtilsMessengerEXT"))};
  if (func != nullptr) {
    func(ctx.instance, ctx.debug_messenger, &vk_alloc_callbacks);
  }
#endif

  log_info("Destroying instance");
  vkDestroyInstance(ctx.instance, &vk_alloc_callbacks);
}