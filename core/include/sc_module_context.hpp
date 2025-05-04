#ifndef SURGE_CORE_MODULE_CONTEXT_HPP
#define SURGE_CORE_MODULE_CONTEXT_HPP

#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_window.hpp"

namespace surge::module {

struct ContextData {
  window::Window window;
  renderer::vk::Context vk_ctx;
};

using Context = ContextData *;

} // namespace surge::module

#endif // SURGE_CORE_MODULE_CONTEXT_HPP