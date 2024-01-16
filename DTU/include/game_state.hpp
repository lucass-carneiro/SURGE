#ifndef SURGE_MODULE_DTU_GAME_STATE
#define SURGE_MODULE_DTU_GAME_STATE

#include "error_types.hpp"

#include <optional>

namespace DTU {

struct game_state {
  using state_load_t = std::optional<DTU::error> (*)();
  using state_unload_t = std::optional<DTU::error> (*)();

  state_load_t state_load{nullptr};
  state_unload_t state_unload{nullptr};
};

} // namespace DTU

#endif // SURGE_MODULE_DTU_GAME_STATE