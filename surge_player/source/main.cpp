#include "module_manager.hpp"
#include "surge_player.hpp"

auto main() noexcept -> int {
  surge::timers::generic_timer t{};
  log_info("Hello world!");
  log_info("Elapsed : %.16f", t.stop());

  t.start();
  auto module{surge::modules::load("./libsurge_module_default.so")};
  if (!module) {
    return 1;
  }
  log_info("Elapsed : %.16f", t.stop());

  while (true) {
    t.start();

    module->draw();
    module->update(0.0);

    log_info("Elapsed : %.16f", t.stop());
  }

  surge::modules::unload(*module);

  return 0;
}