#include "window.hpp"

#include "logging_system/logging_system.hpp"
#include "options.hpp"
#include "safe_ops.hpp"

// clang-format off
#include "opengl/program.hpp"
#include "opengl/uniforms.hpp"

#include <glm/gtc/matrix_transform.hpp>
// clang-format on

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

surge::global_engine_window::~global_engine_window() {
  log_info("Deleting window.");

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  // Calling reset on the window* guarantees that it will be destroyed before
  // glfwTerminate
  window.reset();

  if (window_init_success) {
    glfwTerminate();
  }
}

auto surge::global_engine_window::init() noexcept -> bool {
  log_info("Initializing window");

  // Retrieve, parse and cast configuration values from config script using the main thread VM
  lua_State *L{global_lua_states::get().at(0).get()};
  engine_config = lua_get_engine_config(L);

  if (!engine_config) {
    window_init_success = false;
    return window_init_success;
  }

  // Register GLFW error callback
  glfwSetErrorCallback(glfw_error_callback);

  // Initialize GLFW memory allocator structure;
  // TODO: This is only available in conan 3.4, which conan does not support yet

  // Initialize glfw
  if (glfwInit() != GLFW_TRUE) {
    window_init_success = false;
    return window_init_success;
  }

  // Validate monitor index
  auto monitors = querry_available_monitors();
  if (!monitors.has_value()) {
    glfwTerminate();
    window_init_success = false;
    return window_init_success;
  }

  if (engine_config->window_monitor_index >= monitors.value().second) {
    log_warn("Unable to set window monitor to {} because there are only {} "
             "monitors. Using default monitor index 0",
             engine_config->window_monitor_index, monitors.value().second);
    engine_config->window_monitor_index = 0;
  }

  // GLFW window creation
  log_info("Initializing engine window");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef SURGE_SYSTEM_MacOSX // TODO: Is this macro name correct?
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if (engine_config->windowed) {
    (void)window.release();
    window.reset(glfwCreateWindow(engine_config->window_width, engine_config->window_height,
                                  engine_config->window_name, nullptr, nullptr));
  } else {
    (void)window.release();
    window.reset(glfwCreateWindow(
        engine_config->window_width, engine_config->window_height, engine_config->window_name,
        (monitors.value().first)[engine_config->window_monitor_index], nullptr));
  }

  if (window == nullptr) {
    glfwTerminate();
    window_init_success = false;
    return window_init_success;
  }

  /*******************************
   *       OpenGL Context        *
   *******************************/

  glfwMakeContextCurrent(window.get());
  glfwSwapInterval(1); // TODO: Set vsync via code;

  /*******************************
   *      Input callbacks        *
   *******************************/
  glfwSetKeyCallback(window.get(), glfw_key_callback);
  glfwSetMouseButtonCallback(window.get(), glfw_mouse_button_callback);
  glfwSetScrollCallback(window.get(), glfw_scroll_callback);

  /*******************************
   *          DearImGui          *
   *******************************/

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  // ImGuiIO &io = ImGui::GetIO();
  //   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  //   io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
  ImGui_ImplOpenGL3_Init("#version 130");

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    log_error("Failed to initialize GLAD");
    window.reset();
    glfwTerminate();
    window_init_success = false;
    return window_init_success;
  }

  // Resize callback and viewport creation.
  glfwSetFramebufferSizeCallback(window.get(), framebuffer_size_callback);

  /*******************************
   *       OpenGL Options        *
   *******************************/
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /*******************************
   *           SHADERS           *
   *******************************/
  if (!(std::filesystem::exists(engine_config->root_dir)
        && std::filesystem::is_directory(engine_config->root_dir))) {

    log_error(
        "The path {} in the configuration value \"engine_root_dir\" is not a valid directory.",
        engine_config->root_dir);

    window.reset();
    glfwTerminate();
    window_init_success = false;
    return window_init_success;
  }

  log_info("Compiling shaders");

  sprite_shader = create_program("shaders/sprite.vert", "shaders/sprite.frag");
  if (!sprite_shader) {
    window.reset();
    glfwTerminate();
    window_init_success = false;
    return window_init_success;
  }

  image_shader = create_program("shaders/image.vert", "shaders/image.frag");
  if (!image_shader) {
    window.reset();
    glfwTerminate();
    window_init_success = false;
    return window_init_success;
  }

  /*******************************
   *       VIEW/PROJECTION       *
   *******************************/
  view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

  projection_matrix
      = glm::ortho(0.0f, static_cast<float>(engine_config->window_width),
                   static_cast<float>(engine_config->window_height), 0.0f, 0.0f, 1.1f);

  glUseProgram(*sprite_shader);
  set_uniform(*sprite_shader, "view", view_matrix);
  set_uniform(*sprite_shader, "projection", projection_matrix);

  glUseProgram(*image_shader);
  set_uniform(*image_shader, "view", view_matrix);
  set_uniform(*image_shader, "projection", projection_matrix);

  /*******************************
   *           CURSORS           *
   *******************************/
  if (engine_config->show_cursor) {
    glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  } else {
    glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  }

  window_init_success = true;
  return window_init_success;
}

auto surge::global_engine_window::querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, int>> {

  int count = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&count);

  if (monitors == nullptr) {
    return {};
  }

  log_info("Monitors detected: {}", count);

  for (int i = 0; i < count; i++) {
    int width = 0, height = 0;
    float xscale = 0, yscale = 0;
    int xpos = 0, ypos = 0;
    int w_xpos = 0, w_ypos = 0, w_width = 0, w_height = 0;

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPhysicalSize(monitors[i], &width, &height);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (name == nullptr) {
      return {};
    }

    // clang-format off
    log_info(
        "Properties of monitor {}:\n"
        "  Monitor name: {}.\n"
        "  Physical size (width, height): {}, {}.\n"
        "  Content scale (x, y): {}, {}.\n"
        "  Virtual position: (x, y): {}, {}.\n"
        "  Work area (x, y, width, height): {}, {}, {}, {}.",
        i,
        name,
        width,
        height,
        xscale,
        yscale,
        xpos,
        ypos,
        w_xpos,
        w_ypos,
        w_width,
        w_height
    );
    // clang-format on
  }

  return std::make_pair(monitors, count);
}

void surge::glfw_error_callback(int code, const char *description) noexcept {
  log_error("GLFW error code {}: {}", code, description);
}

void surge::framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}