// clang-format off
#include "state_machine.hpp"
#include "main_menu.hpp"
// clang-format on

#include "player/logging.hpp"

static inline auto state_to_str(DTU::state s) noexcept -> const char * {
  using DTU::state;

  switch (s) {
  case state::no_state:
    return "no state";

  case state::exit_game:
    return "exit game";

  case state::main_menu:
    return "main menu";

  case state::new_game:
    return "new game";

  case state::count:
    return "count";

  default:
    return "unknown state";
  }
}

void DTU::state_machine::push(state s) noexcept {
  if (state_b != no_state) {
    log_warn("Overwriting state %s with %s", state_to_str(state_b), state_to_str(s));
  }
  state_b = s;
}

auto DTU::state_machine::load_a(tdb_t &tdb) noexcept -> std::optional<surge::error> {
  using namespace DTU::state_impl;

  switch (state_a) {

  case state::main_menu:
    return main_menu::load(tdb);

  case state::new_game:
    // TODO;
    return {};

  case no_state:
    return {};

  case exit_game:
    return {};

  case count:
    return {};

  default:
    return {};
  }
  return {};
}

auto DTU::state_machine::unload_a(tdb_t &tdb) noexcept -> std::optional<surge::error> {
  using namespace DTU::state_impl;

  switch (state_a) {

  case state::main_menu:
    return main_menu::unload(tdb);

  case state::new_game:
    // TODO;
    return {};

  case no_state:
    return {};

  case exit_game:
    return {};

  case count:
    return {};

  default:
    return {};
  }
  return {};
}

auto DTU::state_machine::transition(tdb_t &tdb) noexcept -> std::optional<surge::error> {

  const auto a_empty_b_empty{state_a == state::no_state && state_b == state::no_state};
  const auto a_full_b_empty{state_a != state::no_state && state_b == state::no_state};
  const auto a_empty_b_full{state_a == state::no_state && state_b != state::no_state};
  const auto a_full_b_full{state_a != state::no_state && state_b != state::no_state};

  if (a_empty_b_empty || a_full_b_empty) {
    return {};
  } else if (a_empty_b_full) {
    state_a = state_b;
    state_b = state::no_state;
    return load_a(tdb);
  } else if (a_full_b_full) {
    const auto unload_result{unload_a(tdb)};
    if (unload_result) {
      return unload_result;
    } else {
      state_a = state_b;
      state_b = state::no_state;
      return load_a(tdb);
    }
  }

  return {};
}

auto DTU::state_machine::destroy(tdb_t &tdb) noexcept -> std::optional<surge::error> {
  return unload_a(tdb);
}

auto DTU::state_machine::update(GLFWwindow *window, double dt, tdb_t &tdb, sdb_t &sdb,
                                txd_t &txd) noexcept -> std::optional<surge::error> {
  using namespace DTU::state_impl;

  switch (state_a) {

  case state::main_menu:
    return main_menu::update(window, dt, tdb, sdb, txd);

  case state::new_game:
    // TODO;
    return {};

  case state::exit_game:
    return surge::error::normal_exit;

  case state::no_state:
    return {};

  case state::count:
    return {};
  }
  return {};
}