#include "module_manager.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "options.hpp"
#include "timers.hpp"

#include <GLFW/glfw3.h>
#include <cstddef>
#include <cstdint>

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <dlfcn.h>
#else
#  error "Unable to load and unload modules in this OS
#endif

#ifdef SURGE_SYSTEM_IS_POSIX

auto surge::module::load(const char *module_name) noexcept -> handle_t {
  log_info("Loading %s", module_name);

  // Load and get handle
  auto handle{dlopen(module_name, RTLD_LAZY)};
  if (!handle) {
    log_error("Unable to load library %s", dlerror());
    return nullptr;
  }

  // Find callbacks
  (void)dlerror();
  auto on_load_handle{dlsym(handle, "on_load")};
  if (!on_load_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  (void)dlerror();
  auto on_unload_handle{dlsym(handle, "on_unload")};
  if (!on_unload_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  (void)dlerror();
  auto draw_handle{dlsym(handle, "draw")};
  if (!draw_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  (void)dlerror();
  auto update_handle{dlsym(handle, "update")};
  if (!update_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  (void)dlerror();
  auto keyboard_event_handle{dlsym(handle, "keyboard_event")};
  if (!keyboard_event_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  (void)dlerror();
  auto mouse_button_event_handle{dlsym(handle, "mouse_button_event")};
  if (!mouse_button_event_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  (void)dlerror();
  auto mouse_scroll_event_handle{dlsym(handle, "mouse_scroll_event")};
  if (!mouse_scroll_event_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return nullptr;
  }

  return handle;
}

void surge::module::unload(handle_t module_handle) noexcept {
  if (!module_handle) {
    return;
  }

  Dl_info info;
  const auto dladdr_stats{dladdr(dlsym(module_handle, "on_unload"), &info)};

  if (dladdr_stats != 0) {
    log_info("Unloading %s", info.dli_fname);
  }

  const auto result{dlclose(module_handle)};
  if (result != 0) {
    if (dladdr_stats != 0) {
      log_error("Unable to close library %s: %s", info.dli_fname, dlerror());
    } else {
      log_error("Unable to close library %s", dlerror());
    }
  }
}

auto surge::module::reload(GLFWwindow *window, handle_t module_handle) noexcept -> handle_t {
  log_info("Reloading currently loaded module");

  timers::generic_timer t;
  t.start();

  Dl_info info;
  const auto dladdr_stats{dladdr(dlsym(module_handle, "on_unload"), &info)};

  if (dladdr_stats == 0) {
    log_error("Unable to obtain stats for currently loaded module");
    return nullptr;
  }

  const auto module_file_name{allocators::mimalloc::strdup(info.dli_fname)};
  if (!module_file_name) {
    log_error("Unable to allocate duplicate of current module's file name");
    return nullptr;
  }

  // Unload
  on_unload(window, module_handle);
  unload(module_handle);

  // Load
  auto new_handle = load(module_file_name);
  if (!new_handle) {
    allocators::mimalloc::free(module_file_name);
    return nullptr;
  }
  allocators::mimalloc::free(module_file_name);

  on_load(window, new_handle);

  t.stop();
  log_info("Reloading succsesfull in %f s", t.elapsed());
  return new_handle;
}

void surge::module::on_load(GLFWwindow *window, handle_t module_handle) {
  if (!module_handle) {
    return;
  }

  // bind callbacks
  auto keyboard_event{reinterpret_cast<GLFWkeyfun>(dlsym(module_handle, "keyboard_event"))};

  auto mouse_button_event{
      reinterpret_cast<GLFWmousebuttonfun>(dlsym(module_handle, "mouse_button_event"))};

  auto mouse_scroll_event{
      reinterpret_cast<GLFWscrollfun>(dlsym(module_handle, "mouse_scroll_event"))};

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
  }

  // Call on load
  reinterpret_cast<on_load_fun>(dlsym(module_handle, "on_load"))();
}

void surge::module::on_unload(GLFWwindow *window, handle_t module_handle) {
  if (!module_handle) {
    return;
  }

  // Call on unload
  reinterpret_cast<on_load_fun>(dlsym(module_handle, "on_unload"))();

  // Unbind callbacks
  glfwSetKeyCallback(window, nullptr);
  glfwSetMouseButtonCallback(window, nullptr);
  glfwSetScrollCallback(window, nullptr);
}

void surge::module::update(handle_t module_handle, double dt) noexcept {
  if (!module_handle) {
    return;
  }

  reinterpret_cast<update_fun>(dlsym(module_handle, "update"))(dt);
}

void surge::module::draw(handle_t module_handle) noexcept {
  if (!module_handle) {
    return;
  }

  reinterpret_cast<draw_fun>(dlsym(module_handle, "draw"))();
}

#endif