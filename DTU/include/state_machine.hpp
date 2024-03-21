#ifndef SURGE_DTU_STATE_MACHINE_HPP
#define SURGE_DTU_STATE_MACHINE_HPP

// clang-format off
#include "type_aliases.hpp"

#include "player/error_types.hpp"

#include <optional>
// clang-format on

namespace DTU {

using state_t = surge::u32;
enum state : surge::u32 { no_state, exit_game, main_menu, new_game, count };

struct state_machine {

private:
  state state_a{no_state};
  state state_b{no_state};

  auto load_a(tdb_t &tdb) noexcept -> std::optional<surge::error>;
  auto unload_a(tdb_t &tdb) noexcept -> std::optional<surge::error>;

public:
  void push(state s) noexcept;
  auto transition(tdb_t &tdb) noexcept -> std::optional<surge::error>;
  auto destroy(tdb_t &tdb) noexcept -> std::optional<surge::error>;

  auto update(GLFWwindow *window, double dt, tdb_t &tdb, sdb_t &sdb) noexcept
      -> std::optional<surge::error>;
};

} // namespace DTU

#endif // SURGE_DTU_STATE_MACHINE_HPP