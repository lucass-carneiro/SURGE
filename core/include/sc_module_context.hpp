#ifndef SURGE_CORE_MODULE_CONTEXT_HPP
#define SURGE_CORE_MODULE_CONTEXT_HPP

#include "sc_window.hpp"

namespace surge::module {

struct ContextData {
  window::Window window;
};

using Context = ContextData *;

} // namespace surge::module

#endif // SURGE_CORE_MODULE_CONTEXT_HPP