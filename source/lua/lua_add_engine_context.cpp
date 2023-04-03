#include "lua/lua_bindings.hpp"
#include "lua/lua_logs.hpp"
#include "lua/lua_utils.hpp"
#include "lua/lua_wrappers.hpp"
#include "safe_ops.hpp"

void surge::lua_add_engine_context(lua_State *L, std::size_t i) noexcept {
  glog<log_event::message>("Adding engine context to VM {} ({:#x})", i,
                           reinterpret_cast<std::uintptr_t>(L));

  // begin surge table
  lua_newtable(L);
  {
    // begin config table
    lua_newtable(L);
    {
      // begin clear_color table
      lua_newtable(L);
      lua_setfield(L, -2, "clear_color");
      // end clear_color table
    }
    lua_setfield(L, -2, "config");
    // end config table

    // TODO: Warning! This is potentially very dangerous. What happens if the user changes this
    // number? Maybe this can be made more secure.
    lua_add_table_field<lua_String, lua_Integer>(L, "vm_index", safe_cast<long>(i).value_or(0));

    // begin sprite table
    lua_newtable(L);
    {
      // begin sprite_meta table
      lua_newtable(L);
      {
        lua_add_table_field<lua_String, lua_String>(L, "__name", "surge::sprite");
        lua_add_table_field<lua_String, lua_CFunction>(L, "__gc", lua_drop_sprite);
      }
      lua_setfield(L, -2, "sprite_meta");
      // end sprite_meta table

      lua_add_table_field<lua_String, lua_CFunction>(L, "new", lua_new_sprite);
      lua_add_table_field<lua_String, lua_CFunction>(L, "draw", lua_draw_sprite);
      lua_add_table_field<lua_String, lua_CFunction>(L, "scale", lua_scale_sprite);
      lua_add_table_field<lua_String, lua_CFunction>(L, "move", lua_move_sprite);
      lua_add_table_field<lua_String, lua_CFunction>(L, "set_geometry", lua_set_sprite_geometry);
      lua_add_table_field<lua_String, lua_CFunction>(L, "set_sheet_indices", lua_sheet_set_indices);
      lua_add_table_field<lua_String, lua_CFunction>(L, "set_sheet_offsets", lua_sheet_set_offsets);
      lua_add_table_field<lua_String, lua_CFunction>(L, "set_sheet_dimentions",
                                                     lua_sheet_set_dimentions);
    }
    lua_setfield(L, -1, "sprite");

    // begin actor table
    lua_newtable(L);
    {
      // begin actor_meta table
      lua_newtable(L);
      {
        lua_add_table_field<lua_String, lua_String>(L, "__name", "surge::actor");
        lua_add_table_field<lua_String, lua_CFunction>(L, "__gc", lua_drop_actor);
      }
      lua_setfield(L, -2, "actor_meta");
      // end actor_meta table

      lua_add_table_field<lua_String, lua_CFunction>(L, "new", lua_new_actor);
      lua_add_table_field<lua_String, lua_CFunction>(L, "draw", lua_draw_actor);
      lua_add_table_field<lua_String, lua_CFunction>(L, "move", lua_move_actor);
      lua_add_table_field<lua_String, lua_CFunction>(L, "scale", lua_scale_actor);
      lua_add_table_field<lua_String, lua_CFunction>(L, "update", lua_update_actor);
    }
    lua_setfield(L, -2, "actor");
    // end actor table

    // begin log table
    lua_newtable(L);
    {
      lua_add_table_field<lua_String, lua_CFunction>(L, "message", lua_log_message);
      lua_add_table_field<lua_String, lua_CFunction>(L, "warning", lua_log_warning);
      lua_add_table_field<lua_String, lua_CFunction>(L, "error", lua_log_error);
      lua_add_table_field<lua_String, lua_CFunction>(L, "memory", lua_log_memory);
    }
    lua_setfield(L, -2, "log");
    // end log table

    // Tasker functions
    lua_add_table_field<lua_String, lua_CFunction>(L, "send_task_to", lua_send_task_to);
    lua_add_table_field<lua_String, lua_CFunction>(L, "run_task_at", lua_run_task_at);

    // begin input table
    lua_newtable(L);
    {
      // begin action table
      lua_newtable(L);
      {
        lua_add_table_field<lua_String, lua_Integer>(L, "RELEASE", 0);
        lua_add_table_field<lua_String, lua_Integer>(L, "PRESS", 1);
        lua_add_table_field<lua_String, lua_Integer>(L, "REPEAT", 2);
      }
      lua_setfield(L, -2, "action");
      // end action table

      // begin mouse table
      lua_newtable(L);
      {
        lua_add_table_field<lua_String, lua_CFunction>(L, "get_cursor_position",
                                                       lua_get_cursor_pos);

        // begin button table
        lua_newtable(L);
        {
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
        }
        lua_setfield(L, -2, "button");
        // end button table

        // end mouse table
      }
      lua_setfield(L, -2, "mouse");

      // begin keyboard table
      lua_newtable(L);
      {
        lua_add_table_field<lua_String, lua_CFunction>(L, "get_key_state", lua_get_key_state);

        // begin key table
        lua_newtable(L);
        {
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
        }
        lua_setfield(L, -2, "key");
        // end key table

        // Begin modifier table
        lua_newtable(L);
        {
          lua_add_table_field<lua_String, lua_Integer>(L, "MOD_SHIFT", 0x0001);
          lua_add_table_field<lua_String, lua_Integer>(L, "MOD_CONTROL", 0x0002);
          lua_add_table_field<lua_String, lua_Integer>(L, "MOD_ALT", 0x0004);
          lua_add_table_field<lua_String, lua_Integer>(L, "MOD_SUPER", 0x0008);
          lua_add_table_field<lua_String, lua_Integer>(L, "MOD_CAPS_LOCK", 0x0010);
          lua_add_table_field<lua_String, lua_Integer>(L, "MOD_NUM_LOCK", 0x0020);
        }
        lua_setfield(L, -2, "modifier");
        // end modifier table
      }
      lua_setfield(L, -2, "keyboard");
      // end keyboard table
    }
    lua_setfield(L, -2, "input");
    // end input table

    // Actor heading
    lua_newtable(L);
    lua_add_table_field<lua_String, lua_Integer>(L, "N", 0);
    lua_add_table_field<lua_String, lua_Integer>(L, "S", 1);
    lua_add_table_field<lua_String, lua_Integer>(L, "E", 2);
    lua_add_table_field<lua_String, lua_Integer>(L, "W", 3);
    lua_add_table_field<lua_String, lua_Integer>(L, "NE", 4);
    lua_add_table_field<lua_String, lua_Integer>(L, "NE", 5);
    lua_add_table_field<lua_String, lua_Integer>(L, "SE", 6);
    lua_add_table_field<lua_String, lua_Integer>(L, "SW", 7);
    lua_add_table_field<lua_String, lua_Integer>(L, "NONE", 8);
    lua_setfield(L, -2, "actor_heading");
    // end Actor heading
  }
  lua_setglobal(L, "surge");
}