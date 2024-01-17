#ifndef SURGE_MODULE_DTU_GAME_STATE
#define SURGE_MODULE_DTU_GAME_STATE

namespace DTU {

struct game_state {
  using state_load_t = int (*)();
  using state_unload_t = int (*)();
  using state_draw_t = int (*)();

  state_load_t state_load{nullptr};
  state_unload_t state_unload{nullptr};
  state_draw_t state_draw{nullptr};
};

} // namespace DTU

#endif // SURGE_MODULE_DTU_GAME_STATE