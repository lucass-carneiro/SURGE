#include "cli.hpp"

#include "options.hpp"

#include <fmt/core.h>

namespace surge::cli {

// clang-format off
static constexpr const char* LOGO =
	"   d888888o.   8 8888      88 8 888888888o.        ,o888888o.    8 8888888888  \n"
	" .`8888:' `88. 8 8888      88 8 8888    `88.      8888     `88.  8 8888        \n"
	" 8.`8888.   Y8 8 8888      88 8 8888     `88   ,8 8888       `8. 8 8888        \n"
	" `8.`8888.     8 8888      88 8 8888     ,88   88 8888           8 8888        \n"
	"  `8.`8888.    8 8888      88 8 8888.   ,88'   88 8888           8 888888888888\n"
	"   `8.`8888.   8 8888      88 8 888888888P'    88 8888           8 8888        \n"
	"    `8.`8888.  8 8888      88 8 8888`8b        88 8888   8888888 8 8888        \n"
	"8b   `8.`8888. ` 8888     ,8P 8 8888 `8b.      `8 8888       .8' 8 8888        \n"
	"`8b.  ;8.`8888   8888   ,d8P  8 8888   `8b.       8888     ,88'  8 8888        \n"
	" `Y8888P ,88P'    `Y88888P'   8 8888     `88.      `8888888P'    8 888888888888\n";
// clang-format on

} // namespace surge::cli

void surge::cli::draw_logo() noexcept {
#ifdef SURGE_USE_LOG_COLOR
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  fmt::print("\033[1;38;2;220;20;60m{}\033[m", LOGO);
#else
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  fmt::print("{}", LOGO);
#endif
}