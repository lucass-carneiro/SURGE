#include "window.hpp"

#include "logging.hpp"
#include "options.hpp"

#include <GLFW/glfw3.h>

#ifdef SURGE_SYSTEM_Linux
#  define GLFW_EXPOSE_NATIVE_X11
#  include <GLFW/glfw3native.h>
#else
#  error "Unknow native API export macro. See https://www.glfw.org/docs/3.3/group__native.html
#endif

// clang-format off
#include <bgfx/platform.h>
#include <yaml-cpp/yaml.h>
// clang-format on

#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <string>

struct clear_color {
  float r;
  float g;
  float b;
  float a;
};

static void glfw_error_callback(int code, const char *description) noexcept {
  log_error("GLFW erro code %i: %s", code, description);
}

static auto rgba_to_hex(float R, float G, float B, float A) noexcept -> std::uint32_t {
  uint32_t r{static_cast<std::uint32_t>(255.0 * R)};
  uint32_t g{static_cast<std::uint32_t>(255.0 * G)};
  uint32_t b{static_cast<std::uint32_t>(255.0 * B)};
  uint32_t a{static_cast<std::uint32_t>(255.0 * A)};

  return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
}

auto surge::window::init(const char *config_file) noexcept
    -> std::tuple<GLFWwindow *, std::uint32_t, std::uint32_t, std::uint32_t> {
  /******************
   * Config parsing *
   ******************/
  log_info("Parsing config file %s", config_file);

  bgfx::RendererType::Enum rt{};
  bgfx::Resolution res{};
  clear_color ccl{};

  std::uint16_t vendor_Id{0};

  std::string wname{};
  int mi{0};
  bool windowed{true};
  bool cursor{true};

  try {
    const auto cf{YAML::LoadFile(config_file)};

    const auto rbe{cf["renderer"]["backend"].as<std::string>()};
    if (rbe == "Vulkan" || rbe == "vulkan") {
      rt = bgfx::RendererType::Vulkan;
    } else if (rbe == "OpenGL" || rbe == "opengl") {
      rt = bgfx::RendererType::OpenGL;
    } else {
      log_error("Unknow render beckend %s", rbe.c_str());
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    const auto gpu_v{cf["renderer"]["gpu_vendor"].as<std::string>()};
    if (gpu_v == "nVidia" || gpu_v == "nvidia" || gpu_v == "NVIDIA") {
      vendor_Id = BGFX_PCI_ID_NVIDIA;
    } else if (gpu_v == "AMD" || gpu_v == "amd") {
      vendor_Id = BGFX_PCI_ID_AMD;
    } else if (gpu_v == "intel") {
      vendor_Id = BGFX_PCI_ID_INTEL;
    } else {
      log_error("Unknow GPU vendor %s", gpu_v.c_str());
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    res.width = cf["window"]["resolution"]["width"].as<int>();
    res.height = cf["window"]["resolution"]["height"].as<int>();

    if (cf["window"]["VSync"]["enabled"].as<bool>()) {
      res.reset |= BGFX_RESET_VSYNC;
    }

    if (cf["window"]["MSAA"]["enabled"].as<bool>()) {
      const auto level{cf["window"]["MSAA"]["level"].as<int>()};
      switch (level) {
      case 2:
        res.reset |= BGFX_RESET_MSAA_X2;
        break;
      case 4:
        res.reset |= BGFX_RESET_MSAA_X4;
        break;
      case 8:
        res.reset |= BGFX_RESET_MSAA_X8;
        break;
      case 16:
        res.reset |= BGFX_RESET_MSAA_X16;
        break;
      default:
        log_error("Unable to set MSAA level to x%i", level);
        return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
      }
    }

    if (cf["window"]["HDR"]["enabled"].as<bool>()) {
      res.reset |= BGFX_RESET_HDR10;
    }

    ccl.r = cf["renderer"]["clear_color"]["r"].as<float>();
    ccl.g = cf["renderer"]["clear_color"]["g"].as<float>();
    ccl.b = cf["renderer"]["clear_color"]["b"].as<float>();
    ccl.a = cf["renderer"]["clear_color"]["a"].as<float>();

    wname = cf["window"]["name"].as<std::string>();
    mi = cf["window"]["monitor_index"].as<int>();
    windowed = cf["window"]["windowed"].as<bool>();
    cursor = cf["window"]["cursor"].as<bool>();

  } catch (const std::exception &e) {
    log_error("Unable to load %s: %s", config_file, e.what());
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  /*************
   * GLFW init *
   *************/
  log_info("Initializing GLFW");

  glfwSetErrorCallback(glfw_error_callback);

  if (glfwInit() != GLFW_TRUE) {
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  /*****************
   * Monitor query *
   *****************/
  log_info("Querying monitors");

  int mc = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&mc);

  if (monitors == nullptr) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  log_info("Monitors detected: %i", mc);

  for (int i = 0; i < mc; i++) {
    int width = 0, height = 0;
    float xscale = 0, yscale = 0;
    int xpos = 0, ypos = 0;
    int w_xpos = 0, w_ypos = 0, w_width = 0, w_height = 0;

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPhysicalSize(monitors[i], &width, &height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
    }

    log_info("Properties of monitor %i:\n"
             "  Monitor name: %s.\n"
             "  Physical size (width, height): %i, %i.\n"
             "  Content scale (x, y): %f, %f.\n"
             "  Virtual position: (x, y): %i, %i.\n"
             "  Work area (x, y, width, height): %i, %i, %i, %i.",
             i, name, width, height, xscale, yscale, xpos, ypos, w_xpos, w_ypos, w_width, w_height);
  }

  /***************
   * Window init *
   ***************/
  log_info("Initializing engine window");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  GLFWwindow *window = nullptr;

  if (windowed) {
    window = glfwCreateWindow(res.width, res.height, wname.c_str(), nullptr, nullptr);
  } else {
    window = glfwCreateWindow(res.width, res.height, wname.c_str(), monitors[mi < mc ? mi : 0],
                              nullptr);
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  glfwSetInputMode(window, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  /*************
   * BGFX init *
   *************/
  log_info("Initializing BGFX");

  bgfx::PlatformData pd{};

#ifdef SURGE_SYSTEM_Linux
  pd.ndt = glfwGetX11Display();
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  pd.nwh = reinterpret_cast<void *>(static_cast<std::uintptr_t>(glfwGetX11Window(window)));
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }
#endif

  bgfx::Init init_data{};
  init_data.type = rt;
  init_data.vendorId = vendor_Id;
  init_data.platformData = pd;
  init_data.resolution = res;

  // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
  // Most graphics APIs must be used on the same thread that created the window.
  if (rt == bgfx::RendererType::OpenGL) {
    bgfx::renderFrame();
  }

  if (!bgfx::init(init_data)) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     rgba_to_hex(ccl.r, ccl.g, ccl.b, ccl.a), 1.0f, 0);
  bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);

  return std::make_tuple(window, res.width, res.height, res.reset);
}

void surge::window::terminate(GLFWwindow *window) noexcept {
  log_info("Terminating window and renderer");
  bgfx::shutdown();
  glfwDestroyWindow(window);
  glfwTerminate();
}

void surge::window::handle_resize(GLFWwindow *window, std::uint32_t &old_w, std::uint32_t &old_h,
                                  std::uint32_t reset_flags) noexcept {
  int new_w{0}, new_h{0};
  glfwGetWindowSize(window, &new_w, &new_h);

  if (static_cast<std::uint32_t>(new_w) != old_w || static_cast<std::uint32_t>(new_h) != old_h) {
    bgfx::reset(new_w, new_h, reset_flags);
    bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
    old_w = new_w;
    old_h = new_h;
  }
}

auto surge::window::get_dims(GLFWwindow *window) noexcept -> std::tuple<float, float> {
  int ww{0}, wh{0};
  glfwGetWindowSize(window, &ww, &wh);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return std::make_tuple(0.0f, 0.0f);
  } else {
    return std::make_tuple(gsl::narrow_cast<float>(ww), gsl::narrow_cast<float>(wh));
  }
}
