#include "module.hpp"

#include "logging.hpp"

#include <filesystem>

#ifdef SURGE_SYSTEM_Windows

auto surge::module::get_name(handle_t module, std::size_t max_size) noexcept
    -> tl::expected<std::string, module_error> {
  std::string module_name{};
  module_name.reserve(max_size);

  if (GetModuleFileNameA(module, module_name.data(), gsl::narrow_cast<DWORD>(max_size)) == 0) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to retrieve module %p name: %s", module, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(module_error::name_retrival);
  } else {
    return module_name;
  }
}

auto surge::module::load(const char *path) noexcept -> tl::expected<handle_t, module_error> {
  log_info("Loading module %s", path);

  handle_t handle{LoadLibraryA(path)};
  if (!handle) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to load module %s: %s", path, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(module_error::loading);
  } else {
    log_info("Loaded module %s, address %p", path, handle);
    return handle;
  }
}

void surge::module::unload(handle_t module) noexcept {
  if (!module) {
    return;
  }

  log_info("Unloading module %p", module);

  if (FreeLibrary(module) == 0) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to close module %p: %s", module, error_txt);
    LocalFree(error_txt);
  } else {
    log_info("Unloaded module %p", module);
  }
}

auto surge::module::get_api(handle_t module) noexcept -> tl::expected<api, module_error> {
  // on_load
  const auto on_load_addr{GetProcAddress(module, "on_load")};
  if (!on_load_addr) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to obtain handle to on_load in module %p: %s", module, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(module_error::symbol_retrival);
  }

  // on_unload
  const auto on_unload_addr{GetProcAddress(module, "on_unload")};
  if (!on_unload_addr) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to obtain handle to on_unload in module %p: %s", module, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(module_error::symbol_retrival);
  }

  // draw
  const auto draw_addr{GetProcAddress(module, "draw")};
  if (!draw_addr) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to obtain handle to draw in module %p: %s", module, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(module_error::symbol_retrival);
  }

  // update
  const auto update_addr{GetProcAddress(module, "update")};
  if (!update_addr) {
    const auto error_code{GetLastError()};
    LPSTR error_txt{nullptr};
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&error_txt, 0, nullptr);
    log_error("Unable to obtain handle to update in module %p: %s", module, error_txt);
    LocalFree(error_txt);
    return tl::unexpected(module_error::symbol_retrival);
  }

  // clang-format off
  return api{
    reinterpret_cast<on_load_t>(on_load_addr),
    reinterpret_cast<on_unload_t>(on_unload_addr),
    reinterpret_cast<on_unload_t>(draw_addr),
    reinterpret_cast<on_unload_t>(update_addr),
  };
  // clang-format on
}

#else

auto surge::module::get_name(handle_t module, std::size_t) noexcept
    -> tl::expected<std::string, module_error> {

  Dl_info info;
  const auto dladdr_stats{dladdr(dlsym(module, "on_load"), &info)};

  if (dladdr_stats == 0) {
    log_error("Unable to retrieve module %p name.", module);
    return tl::unexpected(module_error::name_retrival);
  } else {
    return std::string(info.dli_fname);
  }
}

auto surge::module::load(const char *path) noexcept -> tl::expected<handle_t, module_error> {
  log_info("Loading module %s", path);

  // Load and get handle
  auto handle{dlopen(path, RTLD_NOW)};
  if (!handle) {
    log_error("Unable to load library %s", dlerror());
    return tl::unexpected(module_error::loading);
  } else {
    log_info("Loaded module %s, address %p", path, handle);
    return handle;
  }
}

void surge::module::unload(handle_t module) noexcept {
  if (!module) {
    return;
  }

  log_info("Unloading module %p", module);

  const auto result{dlclose(module)};
  if (result != 0) {
    log_error("Unable to close module %p: %s", module, dlerror());
  } else {
    log_info("Unloaded module %p", module);
  }
}

auto surge::module::get_api(handle_t module) noexcept -> tl::expected<api, module_error> {
  // on_load
  auto on_load_addr{dlsym(module, "on_load")};
  if (!on_load_addr) {
    log_error("Unable to obtain handle to on_unload in module %p: %s", module, dlerror());
    return tl::unexpected(module_error::symbol_retrival);
  }

  // on_unload
  (void)dlerror();
  auto on_unload_addr{dlsym(module, "on_unload")};
  if (!on_unload_addr) {
    log_error("Unable to obtain handle to on_unload in module %p: %s", module, dlerror());
    return tl::unexpected(module_error::symbol_retrival);
  }

  // draw
  (void)dlerror();
  auto draw_addr{dlsym(module, "draw")};
  if (!draw_addr) {
    log_error("Unable to obtain handle to draw in module %p: %s", module, dlerror());
    return tl::unexpected(module_error::symbol_retrival);
  }

  // update
  (void)dlerror();
  auto update_addr{dlsym(module, "update")};
  if (!update_addr) {
    log_error("Unable to obtain handle to update in module %p: %s", module, dlerror());
    return tl::unexpected(module_error::symbol_retrival);
  }

  // clang-format off
  return api{
    reinterpret_cast<on_load_t>(on_load_addr),    // NOLINT
    reinterpret_cast<on_unload_t>(on_unload_addr), // NOLINT
    reinterpret_cast<draw_t>(draw_addr), // NOLINT
    reinterpret_cast<update_t>(update_addr), // NOLINT
  };
  // clang-format on
}

#endif

auto surge::module::reload(handle_t module) noexcept -> tl::expected<handle_t, module_error> {
  log_info("Reloading currently loaded module");

  // Get module file name
  const auto module_file_name{get_name(module)};
  if (!module_file_name) {
    return tl::unexpected(module_file_name.error());
  }

  // Unload
  unload(module);

  // Check to see if .new exists
  const std::string module_file_name_new{*module_file_name + ".new"};

  std::error_code error;
  if (!std::filesystem::exists(module_file_name_new, error)) {
    log_info("No %s file found. Reloading module without updating", module_file_name_new.c_str());
  } else {
    std::filesystem::rename(module_file_name_new, *module_file_name, error);
    if (error.value() != 0) {
      log_error("Unable to rename %s to %s: %s", module_file_name_new.c_str(),
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