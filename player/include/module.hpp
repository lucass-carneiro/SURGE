#ifndef SURGE_MODULE_HPP
#define SURGE_MODULE_HPP

#include "options.hpp"

#ifdef SURGE_SYSTEM_Windows
// clang-format off
#  include <windows.h>
#  include <libloaderapi.h>
// clang-format on
#else
#  include <dlfcn.h>
#endif

#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <memory>

namespace surge::module {

class owned_module {
public:
  owned_module(const char *path) noexcept;
  ~owned_module();

  [[nodiscard]] auto is_loaded() noexcept -> bool;

  auto on_load() noexcept -> bool;
  auto on_unload() noexcept -> bool;

private:
  using on_load_t = bool (*)(void);
  using on_unload_t = bool (*)(void);

#ifdef SURGE_SYSTEM_Windows
  gsl::owner<HMODULE> module_handle;
#else
  gsl::owner<void *> module_handle;
#endif
  on_load_t on_load_ptr;
  on_unload_t on_unload_ptr;
};

} // namespace surge::module

#endif // SURGE_MODULE_HPP
