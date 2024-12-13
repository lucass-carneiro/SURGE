#include "sc_module.hpp"

#include "sc_logging.hpp"

#include <filesystem>
#include <gsl/gsl-lite.hpp>

#ifdef SURGE_SYSTEM_Windows

auto surge::module::get_name(handle_t module,
                             std::size_t max_size) noexcept -> tl::expected<string, error> {

  auto module_name{string(max_size, '\0')};
  const auto actual_name_size{
      GetModuleFileNameA(module, module_name.data(), gsl::narrow_cast<DWORD>(max_size))};

  if (actual_name_size == 0) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to retrieve module {} name: {}", static_cast<void *>(module), error_txt);
    LocalFree(error_txt);
    return tl::unexpected(error::name_retrival);
  } else {
    module_name.resize(actual_name_size);
    return module_name;
  }
}

auto surge::module::load(const char *path) noexcept -> tl::expected<handle_t, error> {
  log_info("Loading module {}", path);

  handle_t handle{LoadLibraryA(path)};
  if (!handle) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to load module {}: {}", path, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(error::loading);
  } else {
    log_info("Loaded module {}, address {}", path, static_cast<void *>(handle));
    return handle;
  }
}

void surge::module::unload(handle_t module) noexcept {
  if (!module) {
    return;
  }

  log_info("Unloading module {}", static_cast<void *>(module));

  if (FreeLibrary(module) == 0) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to close module {}: {}", static_cast<void *>(module), error_txt);
    LocalFree(error_txt);
  } else {
    log_info("Unloaded module {}", static_cast<void *>(module));
  }
}

static inline auto get_func_addr(surge::module::handle_t module,
                                 const char *func_name) -> tl::expected<FARPROC, surge::error> {
  const auto addr{GetProcAddress(module, func_name)};
  if (!addr) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to obtain handle to function {} in module {}: {}", func_name,
              static_cast<void *>(module), error_txt);
    LocalFree(error_txt);
    return tl::unexpected{surge::error::symbol_retrival};
  } else {
    return addr;
  }
}

auto surge::module::set_module_path() noexcept -> bool {
  char buff[2048];
  GetCurrentDirectoryA(2048, buff);
  log_info("CWD: {}", buff);
  return SetDllDirectoryA(buff);
}

#else

static inline auto get_func_addr(surge::module::handle_t module,
                                 const char *func_name) -> tl::expected<void *, surge::error> {
  (void)dlerror();
  auto addr{dlsym(module, func_name)};
  if (!addr) {
    log_error("Unable to obtain handle to function {} in module {}: {}", func_name, module,
              dlerror());
    return tl::unexpected{surge::error::symbol_retrival};
  } else {
    return addr;
  }
}

auto surge::module::get_name(handle_t module, usize) noexcept -> tl::expected<string, error> {

  Dl_info info;
  const auto dladdr_stats{dladdr(dlsym(module, "on_load"), &info)};

  if (dladdr_stats == 0) {
    log_error("Unable to retrieve module {} name.", module);
    return tl::unexpected(error::name_retrival);
  } else {
    return string{info.dli_fname};
  }
}

auto surge::module::load(const char *path) noexcept -> tl::expected<handle_t, error> {
  log_info("Loading module {}", path);

  // Load and get handle
  auto handle{dlopen(path, RTLD_NOW | RTLD_LOCAL)};
  if (!handle) {
    log_error("Unable to load library {}", dlerror());
    return tl::unexpected(error::loading);
  } else {
    log_info("Loaded module {}, address {}", path, handle);
    return handle;
  }
}

void surge::module::unload(handle_t module) noexcept {
  if (!module) {
    return;
  }

  log_info("Unloading module {}", module);

  const auto result{dlclose(module)};
  if (result != 0) {
    log_error("Unable to close module {}: {}", module, dlerror());
  } else {
    log_info("Unloaded module {}", module);
  }
}

auto surge::module::set_module_path() noexcept -> bool { return true; }

#endif

auto surge::module::get_api(handle_t module) noexcept -> tl::expected<api, error> {
  // on_load
  const auto on_load_addr{get_func_addr(module, "on_load")};
  if (!on_load_addr) {
    return tl::unexpected{on_load_addr.error()};
  }

  // on_unload
  const auto on_unload_addr{get_func_addr(module, "on_unload")};
  if (!on_unload_addr) {
    return tl::unexpected{on_unload_addr.error()};
  }

  // draw
  const auto draw_addr{get_func_addr(module, "draw")};
  if (!draw_addr) {
    return tl::unexpected{draw_addr.error()};
  }

  // update
  const auto update_addr{get_func_addr(module, "update")};
  if (!update_addr) {
    return tl::unexpected{update_addr.error()};
  }

  // keyboard_event
  const auto keyboard_event_addr{get_func_addr(module, "keyboard_event")};
  if (!keyboard_event_addr) {
    return tl::unexpected{keyboard_event_addr.error()};
  }

  // mouse_button_event
  const auto mouse_button_event_addr{get_func_addr(module, "mouse_button_event")};
  if (!mouse_button_event_addr) {
    return tl::unexpected{mouse_button_event_addr.error()};
  }

  // mouse_scroll_event
  const auto mouse_scroll_event_addr{get_func_addr(module, "mouse_scroll_event")};
  if (!mouse_scroll_event_addr) {
    return tl::unexpected{mouse_scroll_event_addr.error()};
  }

  // clang-format off
  return api{
    reinterpret_cast<on_load_t>(on_load_addr.value()),
    reinterpret_cast<on_unload_t>(on_unload_addr.value()),
    reinterpret_cast<draw_t>(draw_addr.value()),
    reinterpret_cast<update_t>(update_addr.value()),
    reinterpret_cast<keyboard_event_t>(keyboard_event_addr.value()),
    reinterpret_cast<mouse_button_event_t>(mouse_button_event_addr.value()),
    reinterpret_cast<mouse_scroll_event_t>(mouse_scroll_event_addr.value())
  };
  // clang-format on
}

auto surge::module::reload(handle_t module) noexcept -> tl::expected<handle_t, error> {
  // Get module file name
  const auto module_file_name{get_name(module)};
  if (!module_file_name) {
    return tl::unexpected(module_file_name.error());
  }

  log_info("Reloading {}", module_file_name->c_str());

  // Unload
  unload(module);

  // Check to see if .new exists
  const auto module_file_name_new{*module_file_name + ".new"};
  log_info("Checking if {} exists", module_file_name_new.c_str());

  std::error_code error;
  if (!std::filesystem::exists(module_file_name_new, error)) {
    log_info("No {} file found. Reloading module without updating", module_file_name_new.c_str());
  } else {
    log_info("{} exists. Replacing old module", module_file_name_new.c_str());
    std::filesystem::rename(module_file_name_new, *module_file_name, error);
    if (error.value() != 0) {
      log_error("Unable to rename {} to {}: {}", module_file_name_new.c_str(),
                module_file_name->c_str(), error.message().c_str());
      return nullptr;
    }
  }

  // Load
  auto new_handle = load(module_file_name->c_str());
  if (!new_handle) {
    return tl::unexpected(new_handle.error());
  }

  return new_handle;
}