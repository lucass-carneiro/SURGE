// clang-format off
// clang-format on

#include "window.hpp"

#include "logging.hpp"
#include "options.hpp"
#include "renderer.hpp"

// clang-format off
#include <yaml-cpp/yaml.h>
// clang-format on

#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <string>
#include <tuple>

static void glfw_error_callback(int code, const char *description) noexcept {
  log_error("GLFW erro code %i: %s", code, description);
}

static void glfw_framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

auto surge::window::init(const char *config_file) noexcept
    -> std::tuple<GLFWwindow *, std::uint32_t, std::uint32_t, renderer::clear_color> {

  /******************
   * Config parsing *
   ******************/
  log_info("Parsing config file %s", config_file);

  std::string wname{};
  int mi{0};
  bool windowed{true};
  bool cursor{true};
  int ww{0}, wh{0};
  bool vsync{true};

  renderer::clear_color ccl{};

  try {
    const auto cf{YAML::LoadFile(config_file)};

    wname = cf["window"]["name"].as<std::string>();
    mi = cf["window"]["monitor_index"].as<int>();
    windowed = cf["window"]["windowed"].as<bool>();
    cursor = cf["window"]["cursor"].as<bool>();

    ww = cf["window"]["resolution"]["width"].as<int>();
    wh = cf["window"]["resolution"]["height"].as<int>();

    vsync = cf["window"]["VSync"]["enabled"].as<bool>();

    ccl = renderer::clear_color{cf["renderer"]["clear_color"]["r"].as<float>(),
                                cf["renderer"]["clear_color"]["g"].as<float>(),
                                cf["renderer"]["clear_color"]["b"].as<float>(),
                                cf["renderer"]["clear_color"]["a"].as<float>()};

  } catch (const std::exception &e) {
    log_error("Unable to load %s: %s", config_file, e.what());
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  /*************
   * GLFW init *
   *************/
  log_info("Initializing GLFW");

  glfwSetErrorCallback(glfw_error_callback);

  if (glfwInit() != GLFW_TRUE) {
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  /*****************
   * Monitor query *
   *****************/
  log_info("Querying monitors");

  int mc = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&mc);

  if (monitors == nullptr) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
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
      return std::make_tuple(nullptr, 0, 0, ccl);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, ccl);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, ccl);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, ccl);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, ccl);
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

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  // TODO: Is this macro name correct?
#ifdef SURGE_SYSTEM_MacOSX
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }
#endif

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  GLFWwindow *window = nullptr;

  if (windowed) {
    window = glfwCreateWindow(ww, wh, wname.c_str(), nullptr, nullptr);
  } else {
    window = glfwCreateWindow(ww, wh, wname.c_str(), monitors[mi < mc ? mi : 0], nullptr);
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  log_info("Engine window created");

  /*******************************
   *           CURSORS           *
   *******************************/
  log_info("Setting cursor mode");

  glfwSetInputMode(window, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  /***********************
   * OpenGL context init *
   ***********************/
  log_info("Initializing OpenGL");

  glfwMakeContextCurrent(window);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  if (vsync) {
    glfwSwapInterval(1);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return std::make_tuple(nullptr, 0, 0, ccl);
    }
  }

  /********
   * GLAD *
   ********/
  log_info("Initializing GLAD");

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    log_error("Failed to initialize GLAD");
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, ccl);
  }

  /******************
   * OpenGL options *
   ******************/
  renderer::enable(renderer::capability::depth_test);
  renderer::enable(renderer::capability::blend);
  renderer::blend_function(renderer::blend_src::alpha, renderer::blend_dest::one_minus_src_alpha);

  return std::make_tuple(window, ww, wh, ccl);
}

void surge::window::terminate(GLFWwindow *window) noexcept {
  log_info("Terminating window and renderer");
  glfwDestroyWindow(window);
  glfwTerminate();
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
