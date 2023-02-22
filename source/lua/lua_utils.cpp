#include "lua/lua_utils.hpp"

// clang-format off
#include "lua/lua_bindings.hpp"
#include "lua/lua_logs.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_wrappers.hpp"
// clang-format on

#include "file.hpp"
#include "safe_ops.hpp"
#include "thread_allocators.hpp"

void surge::push_engine_config_at(std::size_t i) noexcept {
  auto L = global_lua_states::get().at(i).get();

  // begin surge table
  lua_newtable(L);

  // TODO: Warning! This is potentially very dangerous. What happens if the user changes this
  // number? Maybe this can be made more secure.
  lua_add_table_field<lua_String, lua_Integer>(L, "vm_index", safe_cast<long>(i).value_or(0));

  // begin clear_color array
  lua_newtable(L);
  lua_push_array(L, std::array<lua_Number, 4>{0.0, 0.0, 0.0, 1.0});
  lua_setfield(L, -2, "clear_color");
  // end clear_color table

  // begin sprite_meta table
  lua_newtable(L);
  lua_add_table_field<lua_String, lua_String>(L, "__name", "surge::sprite");
  lua_add_table_field<lua_String, lua_CFunction>(L, "__gc", lua_drop_sprite);
  lua_setfield(L, -2, "sprite_meta");
  // end sprite_meta table

  // begin actor_meta table
  lua_newtable(L);
  lua_add_table_field<lua_String, lua_String>(L, "__name", "surge::actor");
  lua_add_table_field<lua_String, lua_CFunction>(L, "__gc", lua_drop_actor);
  lua_setfield(L, -2, "actor_meta");
  // end actor_meta table

  // Log functions
  lua_add_table_field<lua_String, lua_CFunction>(L, "log_message", lua_log_message);
  lua_add_table_field<lua_String, lua_CFunction>(L, "log_warning", lua_log_warning);
  lua_add_table_field<lua_String, lua_CFunction>(L, "log_error", lua_log_error);
  lua_add_table_field<lua_String, lua_CFunction>(L, "log_memory", lua_log_memory);

  // Sprite functions
  lua_add_table_field<lua_String, lua_CFunction>(L, "load_sprite", lua_load_sprite);
  lua_add_table_field<lua_String, lua_CFunction>(L, "draw_sprite", lua_draw_sprite);
  lua_add_table_field<lua_String, lua_CFunction>(L, "scale_sprite", lua_scale_sprite);
  lua_add_table_field<lua_String, lua_CFunction>(L, "move_sprite", lua_move_sprite);
  lua_add_table_field<lua_String, lua_CFunction>(L, "set_sprite_geometry", lua_set_sprite_geometry);
  lua_add_table_field<lua_String, lua_CFunction>(L, "sheet_set_indices", lua_sheet_set_indices);
  lua_add_table_field<lua_String, lua_CFunction>(L, "sheet_set_offsets", lua_sheet_set_offsets);
  lua_add_table_field<lua_String, lua_CFunction>(L, "sheet_set_dimentions",
                                                 lua_sheet_set_dimentions);

  // Actor functions
  lua_add_table_field<lua_String, lua_CFunction>(L, "new_actor", lua_new_actor);
  lua_add_table_field<lua_String, lua_CFunction>(L, "draw_actor", lua_draw_actor);
  lua_add_table_field<lua_String, lua_CFunction>(L, "set_actor_animation", lua_set_actor_animation);
  lua_add_table_field<lua_String, lua_CFunction>(L, "set_actor_geometry", lua_set_actor_geometry);
  lua_add_table_field<lua_String, lua_CFunction>(L, "set_actor_position", lua_set_actor_position);
  lua_add_table_field<lua_String, lua_CFunction>(L, "set_actor_anchor_point",
                                                 lua_set_actor_anchor_point);
  lua_add_table_field<lua_String, lua_CFunction>(L, "advance_actor_frame", lua_advance_actor_frame);
  lua_add_table_field<lua_String, lua_CFunction>(L, "move_actor", lua_move_actor);
  lua_add_table_field<lua_String, lua_CFunction>(L, "scale_actor", lua_scale_actor);
  lua_add_table_field<lua_String, lua_CFunction>(L, "toggle_actor_h_flip", lua_actor_toggle_h_flip);
  lua_add_table_field<lua_String, lua_CFunction>(L, "toggle_actor_v_flip", lua_actor_toggle_v_flip);

  // Tasker functions
  lua_add_table_field<lua_String, lua_CFunction>(L, "send_task_to", lua_send_task_to);
  lua_add_table_field<lua_String, lua_CFunction>(L, "run_task_at", lua_run_task_at);

  // Mouse functions
  lua_add_table_field<lua_String, lua_CFunction>(L, "get_cursor_pos", lua_get_cursor_pos);

  // begin keyboard_key table
  lua_newtable(L);
  lua_add_table_field<lua_String, lua_Integer>(L, "UNKNOWN", -1);
  lua_add_table_field<lua_String, lua_Integer>(L, "SPACE", 32);
  lua_add_table_field<lua_String, lua_Integer>(L, "APOSTROPHE", 39);
  lua_add_table_field<lua_String, lua_Integer>(L, "COMMA", 44);
  lua_add_table_field<lua_String, lua_Integer>(L, "MINUS", 45);
  lua_add_table_field<lua_String, lua_Integer>(L, "PERIOD", 46);
  lua_add_table_field<lua_String, lua_Integer>(L, "SLASH", 47);
  lua_add_table_field<lua_String, lua_Integer>(L, "0", 48);
  lua_add_table_field<lua_String, lua_Integer>(L, "1", 49);
  lua_add_table_field<lua_String, lua_Integer>(L, "2", 50);
  lua_add_table_field<lua_String, lua_Integer>(L, "3", 51);
  lua_add_table_field<lua_String, lua_Integer>(L, "4", 52);
  lua_add_table_field<lua_String, lua_Integer>(L, "5", 53);
  lua_add_table_field<lua_String, lua_Integer>(L, "6", 54);
  lua_add_table_field<lua_String, lua_Integer>(L, "7", 55);
  lua_add_table_field<lua_String, lua_Integer>(L, "8", 56);
  lua_add_table_field<lua_String, lua_Integer>(L, "9", 57);
  lua_add_table_field<lua_String, lua_Integer>(L, "SEMICOLON", 59);
  lua_add_table_field<lua_String, lua_Integer>(L, "EQUAL", 61);
  lua_add_table_field<lua_String, lua_Integer>(L, "A", 65);
  lua_add_table_field<lua_String, lua_Integer>(L, "B", 66);
  lua_add_table_field<lua_String, lua_Integer>(L, "C", 67);
  lua_add_table_field<lua_String, lua_Integer>(L, "D", 68);
  lua_add_table_field<lua_String, lua_Integer>(L, "E", 69);
  lua_add_table_field<lua_String, lua_Integer>(L, "F", 70);
  lua_add_table_field<lua_String, lua_Integer>(L, "G", 71);
  lua_add_table_field<lua_String, lua_Integer>(L, "H", 72);
  lua_add_table_field<lua_String, lua_Integer>(L, "I", 73);
  lua_add_table_field<lua_String, lua_Integer>(L, "J", 74);
  lua_add_table_field<lua_String, lua_Integer>(L, "K", 75);
  lua_add_table_field<lua_String, lua_Integer>(L, "L", 76);
  lua_add_table_field<lua_String, lua_Integer>(L, "M", 77);
  lua_add_table_field<lua_String, lua_Integer>(L, "N", 78);
  lua_add_table_field<lua_String, lua_Integer>(L, "O", 79);
  lua_add_table_field<lua_String, lua_Integer>(L, "P", 80);
  lua_add_table_field<lua_String, lua_Integer>(L, "Q", 81);
  lua_add_table_field<lua_String, lua_Integer>(L, "R", 82);
  lua_add_table_field<lua_String, lua_Integer>(L, "S", 83);
  lua_add_table_field<lua_String, lua_Integer>(L, "T", 84);
  lua_add_table_field<lua_String, lua_Integer>(L, "U", 85);
  lua_add_table_field<lua_String, lua_Integer>(L, "V", 86);
  lua_add_table_field<lua_String, lua_Integer>(L, "W", 87);
  lua_add_table_field<lua_String, lua_Integer>(L, "X", 88);
  lua_add_table_field<lua_String, lua_Integer>(L, "Y", 89);
  lua_add_table_field<lua_String, lua_Integer>(L, "Z", 90);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT_BRACKET", 91);
  lua_add_table_field<lua_String, lua_Integer>(L, "BACKSLASH", 92);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT_BRACKET", 93);
  lua_add_table_field<lua_String, lua_Integer>(L, "GRAVE_ACCENT", 96);
  lua_add_table_field<lua_String, lua_Integer>(L, "WORLD_1", 161);
  lua_add_table_field<lua_String, lua_Integer>(L, "WORLD_2", 162);
  lua_add_table_field<lua_String, lua_Integer>(L, "ESCAPE", 256);
  lua_add_table_field<lua_String, lua_Integer>(L, "ENTER", 257);
  lua_add_table_field<lua_String, lua_Integer>(L, "TAB", 258);
  lua_add_table_field<lua_String, lua_Integer>(L, "BACKSPACE", 259);
  lua_add_table_field<lua_String, lua_Integer>(L, "INSERT", 260);
  lua_add_table_field<lua_String, lua_Integer>(L, "DELETE", 261);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT", 262);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT", 263);
  lua_add_table_field<lua_String, lua_Integer>(L, "DOWN", 264);
  lua_add_table_field<lua_String, lua_Integer>(L, "UP", 265);
  lua_add_table_field<lua_String, lua_Integer>(L, "PAGE_UP", 266);
  lua_add_table_field<lua_String, lua_Integer>(L, "PAGE_DOWN", 267);
  lua_add_table_field<lua_String, lua_Integer>(L, "HOME", 268);
  lua_add_table_field<lua_String, lua_Integer>(L, "END", 269);
  lua_add_table_field<lua_String, lua_Integer>(L, "CAPS_LOCK", 280);
  lua_add_table_field<lua_String, lua_Integer>(L, "SCROLL_LOCK", 281);
  lua_add_table_field<lua_String, lua_Integer>(L, "NUM_LOCK", 282);
  lua_add_table_field<lua_String, lua_Integer>(L, "PRINT_SCREEN", 283);
  lua_add_table_field<lua_String, lua_Integer>(L, "PAUSE", 284);
  lua_add_table_field<lua_String, lua_Integer>(L, "F1", 290);
  lua_add_table_field<lua_String, lua_Integer>(L, "F2", 291);
  lua_add_table_field<lua_String, lua_Integer>(L, "F3", 292);
  lua_add_table_field<lua_String, lua_Integer>(L, "F4", 293);
  lua_add_table_field<lua_String, lua_Integer>(L, "F5", 294);
  lua_add_table_field<lua_String, lua_Integer>(L, "F6", 295);
  lua_add_table_field<lua_String, lua_Integer>(L, "F7", 296);
  lua_add_table_field<lua_String, lua_Integer>(L, "F8", 297);
  lua_add_table_field<lua_String, lua_Integer>(L, "F9", 298);
  lua_add_table_field<lua_String, lua_Integer>(L, "F10", 299);
  lua_add_table_field<lua_String, lua_Integer>(L, "F11", 300);
  lua_add_table_field<lua_String, lua_Integer>(L, "F12", 301);
  lua_add_table_field<lua_String, lua_Integer>(L, "F13", 302);
  lua_add_table_field<lua_String, lua_Integer>(L, "F14", 303);
  lua_add_table_field<lua_String, lua_Integer>(L, "F15", 304);
  lua_add_table_field<lua_String, lua_Integer>(L, "F16", 305);
  lua_add_table_field<lua_String, lua_Integer>(L, "F17", 306);
  lua_add_table_field<lua_String, lua_Integer>(L, "F18", 307);
  lua_add_table_field<lua_String, lua_Integer>(L, "F19", 308);
  lua_add_table_field<lua_String, lua_Integer>(L, "F20", 309);
  lua_add_table_field<lua_String, lua_Integer>(L, "F21", 310);
  lua_add_table_field<lua_String, lua_Integer>(L, "F22", 311);
  lua_add_table_field<lua_String, lua_Integer>(L, "F23", 312);
  lua_add_table_field<lua_String, lua_Integer>(L, "F24", 313);
  lua_add_table_field<lua_String, lua_Integer>(L, "F25", 314);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_0", 320);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_1", 321);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_2", 322);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_3", 323);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_4", 324);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_5", 325);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_6", 326);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_7", 327);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_8", 328);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_9", 329);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_DECIMAL", 330);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_DIVIDE", 331);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_MULTIPLY", 332);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_SUBTRACT", 333);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_ADD", 334);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_ENTER", 335);
  lua_add_table_field<lua_String, lua_Integer>(L, "KP_EQUAL", 336);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT_SHIFT", 340);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT_CONTROL", 341);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT_ALT", 342);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT_SUPER", 343);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT_SHIFT", 344);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT_CONTROL", 345);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT_ALT", 346);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT_SUPER", 347);
  lua_add_table_field<lua_String, lua_Integer>(L, "MENU", 348);
  lua_add_table_field<lua_String, lua_Integer>(L, "LAST", 348);
  lua_setfield(L, -2, "keyboard_key");
  // end keyboard_key table

  // Begin modifier_bits table
  lua_newtable(L);
  lua_add_table_field<lua_String, lua_Integer>(L, "MOD_SHIFT", 0x0001);
  lua_add_table_field<lua_String, lua_Integer>(L, "MOD_CONTROL", 0x0002);
  lua_add_table_field<lua_String, lua_Integer>(L, "MOD_ALT", 0x0004);
  lua_add_table_field<lua_String, lua_Integer>(L, "MOD_SUPER", 0x0008);
  lua_add_table_field<lua_String, lua_Integer>(L, "MOD_CAPS_LOCK", 0x0010);
  lua_add_table_field<lua_String, lua_Integer>(L, "MOD_NUM_LOCK", 0x0020);
  lua_setfield(L, -2, "modifier_bits");
  // end modifier_bits table

  // Begin input_action table
  lua_newtable(L);
  lua_add_table_field<lua_String, lua_Integer>(L, "RELEASE", 0);
  lua_add_table_field<lua_String, lua_Integer>(L, "PRESS", 1);
  lua_add_table_field<lua_String, lua_Integer>(L, "REPEAT", 2);
  lua_setfield(L, -2, "input_action");
  // end input_action table

  // Mouse button table
  lua_newtable(L);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_1", 0);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_2", 1);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_3", 2);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_4", 3);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_5", 4);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_6", 5);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_7", 6);
  lua_add_table_field<lua_String, lua_Integer>(L, "BUTTON_8", 7);
  lua_add_table_field<lua_String, lua_Integer>(L, "LAST", 7);
  lua_add_table_field<lua_String, lua_Integer>(L, "LEFT", 0);
  lua_add_table_field<lua_String, lua_Integer>(L, "RIGHT", 1);
  lua_add_table_field<lua_String, lua_Integer>(L, "MIDDLE", 2);
  lua_setfield(L, -2, "mouse_button");
  // end Mouse button table

  lua_setglobal(L, "surge");
  // end surge table
}

auto surge::get_lua_engine_config(lua_State *L) noexcept -> std::optional<lua_engine_config> {
  lua_engine_config config{};

  const auto stack_top{lua_gettop(L)};

  lua_getglobal(L, "surge");

  lua_getfield(L, -1, "window_width");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field window_width is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_width = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_height");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field window_height is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_height = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_name");
  if (!lua_isstring(L, -1)) {
    glog<log_event::error>("The value stored in the field window_name is not a string");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_name = lua_tostring(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "windowed");
  if (!lua_isboolean(L, -1)) {
    glog<log_event::error>("The value stored in the field windowed is not a boolean");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.windowed = static_cast<lua_Boolean>(lua_toboolean(L, -1));
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_monitor_index");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field window_monitor_index is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_monitor_index = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "show_cursor");
  if (!lua_isboolean(L, -1)) {
    glog<log_event::error>("The value stored in the field show_cursor is not a boolean");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.show_cursor = static_cast<lua_Boolean>(lua_toboolean(L, -1));
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "show_debug_objects");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field show_debug_objects is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.show_debug_objects = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "engine_root_dir");
  if (!lua_isstring(L, -1)) {
    glog<log_event::error>("The value stored in the field engine_root_dir is not a string");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.root_dir = lua_tostring(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "clear_color");
  if (!lua_istable(L, -1)) {
    glog<log_event::error>("The value stored in the field clear_color is not a table");
    lua_settop(L, stack_top);
    return {};

  } else {

    for (int i = 1; auto &color : config.clear_color) {
      lua_rawgeti(L, -1, i);

      if (lua_isnoneornil(L, -1) || !lua_isnumber(L, -1)) {
        glog<log_event::error>(
            "The value stored in the field clear_color[{}] is nill, none or not a number", i);
        lua_settop(L, stack_top);
        return {};
      }

      color = lua_tonumber(L, -1);
      lua_pop(L, 1);
      i++;
    }

    lua_pop(L, 1);
  }

  lua_settop(L, stack_top);

  return config;
}

struct file_handle {
  surge::load_file_return_t opt_file_span;
  bool file_read{false};
};

auto surge::lua_reader(lua_State *, void *user_data, std::size_t *chunck_size) noexcept -> const
    char * {
  auto handle{static_cast<file_handle *>(user_data)};

  if (!handle->file_read) {
    *chunck_size = handle->opt_file_span.value().size();
    handle->file_read = true;
    return static_cast<const char *>(static_cast<void *>(handle->opt_file_span.value().data()));
  } else {
    *chunck_size = 0;
    return nullptr;
  }
}

auto surge::lua_message_handler(lua_State *L) noexcept -> int {
  const char *msg = lua_tostring(L, 1);

  // is error object not a string?
  if (msg == nullptr) {

    // does it have a metamethod that produces a string?
    if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)

      // that is the message
      return 1;
    else
      // I have no other choice here.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
      msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
  }

  // append a standard traceback
  luaL_traceback(L, L, msg, 1);

  // return the traceback
  return 1;
}

auto surge::do_file_at(std::size_t i, const std::filesystem::path &path) noexcept -> bool {
  // See example implementation from the Lua interpreter here:
  // https://www.lua.org/source/5.4/lua.c.html#msghandler

  glog<log_event::message>("Doing Lua script {} using index {}", path.c_str(), i);

  auto L{global_lua_states::get().at(i).get()};
  auto &alloc{global_thread_allocators::get().at(i)};

  // Step 1: load file into the thread's stack and construct a file_handle object to pass to
  // lua_reader
  file_handle handle{load_file(alloc.get(), path, ".lua"), false};
  if (!handle.opt_file_span) {
    return false;
  }

  // Step 2: Load Lua chunk
  const auto lua_laod_stats{lua_load(L, &lua_reader, static_cast<void *>(&handle), path.c_str())};
  if (lua_laod_stats != 0) {
    glog<log_event::error>("Error while loading Lua script: {}", lua_tostring(L, -1));

    // Step 2.1: free allocated file
    alloc->free(static_cast<void *>((*handle.opt_file_span).data()));

    return false;
  }

  // Step 3: Push the message handler onto the stack
  const auto stack_base{lua_gettop(L)};
  lua_pushcfunction(L, lua_message_handler);
  lua_insert(L, stack_base);

  // Step 4: pCall the lua script
  const auto lua_pcall_stats{lua_pcall(L, 0, LUA_MULTRET, stack_base)};
  lua_remove(L, stack_base);

  if (lua_pcall_stats != 0) {
    glog<log_event::error>("Error while executing Lua script:\n{}", lua_tostring(L, -1));

    // Step 4.1: free allocated file
    alloc->free(static_cast<void *>((*handle.opt_file_span).data()));

    return false;
  }

  // Step 5: free allocated file
  alloc->free(static_cast<void *>((*handle.opt_file_span).data()));

  return true;
}