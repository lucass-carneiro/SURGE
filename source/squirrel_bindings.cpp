#include "squirrel_bindings.hpp"

#include "log.hpp"

#include <cstdarg>
#include <cstdio>
#include <squirrel.h>
#include <type_traits>

surge::squirrel_vm::squirrel_vm(SQInteger sz)
    : initial_stack_size(sz), virtual_machine(sq_open(initial_stack_size)) {
  sqstd_seterrorhandlers(virtual_machine);
  sq_setprintfunc(virtual_machine, squirrel_print_function, squirrel_error_function);
}

surge::squirrel_vm::~squirrel_vm() { sq_close(virtual_machine); }

void surge::squirrel_print_function(HSQUIRRELVM, const SQChar *s, ...) {

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::printf("\033[38;2;96;57;19m%s - SURGE Squirrel VM message:\033[0m ",
              get_datetime_string().c_str());

  std::va_list arglist;       // NOLINT(cppcoreguidelines-pro-type-vararg)
  va_start(arglist, s);       // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  std::scvprintf(s, arglist); // NOLINT
  va_end(arglist);            // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

  std::putchar('\n');
}

void surge::squirrel_error_function(HSQUIRRELVM, const SQChar *s, ...) {
  std::va_list arglist;       // NOLINT(cppcoreguidelines-pro-type-vararg)
  va_start(arglist, s);       // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  std::scvprintf(s, arglist); // NOLINT
  va_end(arglist);            // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
}

void surge::call_surge_callback(HSQUIRRELVM v, const SQChar *function_name) {
  // Save stack size to restore the stack state after the call
  const auto stack_top = sq_gettop(v);

  // get the surge table
  sq_pushroottable(v);
  sq_pushstring(v, _SC("surge"), -1);
  sq_get(v, -2);

  // Try to call the function
  sq_pushstring(v, function_name, -1);
  if (SQ_SUCCEEDED(sq_get(v, -2))) {
    sq_pushroottable(v); // This
    sq_call(v, 1, 0, 1);
  }

  // restores the original stack size
  sq_settop(v, stack_top);
}

auto surge::push_engine_context(HSQUIRRELVM v, const std::filesystem::path &config_script) -> bool {
  bool status = false;
  const auto top = sq_gettop(v);

  sq_pushroottable(v);

  // begin "surge" table
  sq_pushstring(v, _SC("surge"), -1);
  sq_newtable(v);

  sq_pushstring(v, _SC("window_width"), -1);
  sq_pushinteger(v, 800);
  sq_newslot(v, -3, SQBool{false});

  sq_pushstring(v, _SC("window_height"), -1);
  sq_pushinteger(v, 600);
  sq_newslot(v, -3, SQBool{false});

  sq_pushstring(v, _SC("window_name"), -1);
  sq_pushstring(v, _SC("SURGE Window"), -1);
  sq_newslot(v, -3, SQBool{false});

  sq_pushstring(v, _SC("windowed"), -1);
  sq_pushbool(v, SQBool{true});
  sq_newslot(v, -3, SQBool{false});

  sq_pushstring(v, _SC("window_monitor_index"), -1);
  sq_pushinteger(v, 0);
  sq_newslot(v, -3, SQBool{false});

  // end "surge" table
  sq_newslot(v, -3, SQFalse);

  if (SQ_FAILED(sqstd_dofile(v, config_script.c_str(), false, true))) {
    log_all<log_event::error>("An error ocurred while processing {}", config_script.string());
    status = false;
  } else {
    status = true;
  }

  // Restore stack frame
  sq_settop(v, top);

  return status;
}