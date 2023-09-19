#include "module.hpp"

#include "logging.hpp"

#ifdef SURGE_SYSTEM_Windows
auto surge::module::get_name(const handle_t &module, std::size_t max_size) noexcept
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

void surge::module::unload(handle_t &module) noexcept {
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

auto surge::module::get_api(const handle_t &module) noexcept -> tl::expected<api, module_error> {
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

  return api{reinterpret_cast<on_load_t>(on_load_addr),
             reinterpret_cast<on_unload_t>(on_unload_addr)};
}
#endif