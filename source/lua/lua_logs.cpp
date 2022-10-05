// clang-format off
#include "options.hpp"
// clang-format on

#include "lua/lua_logs.hpp"

#include <exception>
#include <iostream>

void surge::vm_colored_print(lua_State *L, fmt::string_view banner,
                             const fmt::text_style &style) noexcept {
  try {
    // clang-format off
      fmt::print(
                #ifdef SURGE_USE_LOG_COLOR
                 style,
                #endif
                "SURGE Lua VM {}: ",
                banner
      );
    // clang-format on

    int nargs = lua_gettop(L);

    for (int i = 1; i <= nargs; ++i) {
      if (lua_isboolean(L, i)) {
        fmt::print("{}", static_cast<bool>(lua_toboolean(L, i)));
      } else if (lua_isnil(L, i)) {
        fmt::print("nill");
      } else {
        fmt::print("{}", lua_tostring(L, i));
      }
    }

    fmt::print("\n");

  } catch (const std::exception &e) {
    std::cout << "Error while invonking fmt: " << e.what() << std::endl;
  }
}