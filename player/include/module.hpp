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
#include <string>
#include <tl/expected.hpp>

namespace surge::module {

enum class module_error { loading, name_retrival, symbol_retrival };

#ifdef SURGE_SYSTEM_Windows
using handle_t = HMODULE;
#else
using handle_t = void *;
#endif

using on_load_t = bool (*)();
using on_unload_t = bool (*)();

struct api {
  on_load_t on_load;
  on_unload_t on_unload;
};

auto get_name(handle_t module, std::size_t max_size = 256) noexcept
    -> tl::expected<std::string, module_error>;

auto load(const char *path) noexcept -> tl::expected<handle_t, module_error>;
void unload(handle_t module) noexcept;

auto get_api(handle_t module) noexcept -> tl::expected<api, module_error>;

} // namespace surge::module

#endif // SURGE_MODULE_HPP
