#include "module.hpp"

#ifdef SURGE_SYSTEM_Windows

surge::module::owned_module::owned_module(const char *path) noexcept
    : module_handle{LoadLibrary(path)},
      on_load_ptr{reinterpret_cast<on_load_t>(GetProcAddress(module_handle, "on_load"))},
      on_unload_ptr{reinterpret_cast<on_unload_t>(GetProcAddress(module_handle, "on_unload"))} {
  // TODO handle load errors
}

surge::module::owned_module::~owned_module() {
  on_unload();
  FreeLibrary(module_handle);
}

#endif

auto surge::module::owned_module::on_load() noexcept -> bool { return on_load_ptr(); }
auto surge::module::owned_module::on_unload() noexcept -> bool { return on_unload_ptr(); }