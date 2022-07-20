#include "squirrel_bindings.hpp"

#include "log.hpp"

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <squirrel.h>
#include <type_traits>

surge::squirrel_vm::squirrel_vm(SQInteger stack_size) noexcept
    : initial_stack_size(stack_size),
      virtual_machine(sq_open(initial_stack_size)) {

  sqstd_seterrorhandlers(virtual_machine);

  sq_setprintfunc(virtual_machine, squirrel_print_function,
                  squirrel_error_function);
}

surge::squirrel_vm::~squirrel_vm() noexcept { sq_close(virtual_machine); }

void surge::squirrel_print_function(HSQUIRRELVM, const SQChar *s,
                                    ...) noexcept {

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::printf("\033[38;2;96;57;19m%s - SURGE Squirrel VM message:\033[0m ",
              get_datetime_string().c_str());

  std::va_list arglist; // NOLINT(cppcoreguidelines-pro-type-vararg)
  va_start(arglist,
           s); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  std::scvprintf(s, arglist); // NOLINT
  va_end(
      arglist); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

  std::putchar('\n');
}

void surge::squirrel_error_function(HSQUIRRELVM, const SQChar *s,
                                    ...) noexcept {
  std::va_list arglist; // NOLINT(cppcoreguidelines-pro-type-vararg)
  va_start(arglist,
           s); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  std::scvprintf(s, arglist); // NOLINT
  va_end(
      arglist); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
}

auto surge::squirrel_vm::call_callback(const SQChar *function_name) noexcept
    -> bool {
  std::lock_guard<std::mutex> lock(vm_mutex);
  bool status = true;

  // Save stack size to restore the stack state after the call
  const auto stack_top = sq_gettop(virtual_machine);

  // get the surge table
  sq_pushroottable(virtual_machine);
  sq_pushstring(virtual_machine, _SC("surge"), -1);
  sq_get(virtual_machine, -2);

  // Try to call the function
  sq_pushstring(virtual_machine, function_name, -1);
  if (SQ_SUCCEEDED(sq_get(virtual_machine, -2))) {
    sq_pushroottable(virtual_machine); // This
    sq_call(virtual_machine, 1, 0, 1);
    status = true;
  } else {
    log_all<log_event::warning>("The virtual machine was unable to call {}.",
                                function_name);
    status = false;
  }

  // restores the original stack size
  sq_settop(virtual_machine, stack_top);

  return status;
}

auto surge::squirrel_vm::load_context(
    const std::filesystem::path &config_script) noexcept -> bool {
  std::lock_guard<std::mutex> lock(vm_mutex);
  bool status = true;

  const auto top = sq_gettop(virtual_machine);

  sq_pushroottable(virtual_machine);

  // begin "surge" table
  sq_pushstring(virtual_machine, _SC("surge"), -1);
  sq_newtable(virtual_machine);

  sq_pushstring(virtual_machine, _SC("window_width"), -1);
  sq_pushinteger(virtual_machine, 800);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("window_height"), -1);
  sq_pushinteger(virtual_machine, 600);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("window_name"), -1);
  sq_pushstring(virtual_machine, _SC("SURGE Window"), -1);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("windowed"), -1);
  sq_pushbool(virtual_machine, SQBool{true});
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("window_monitor_index"), -1);
  sq_pushinteger(virtual_machine, 0);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("clear_color_r"), -1);
  sq_pushfloat(virtual_machine, 0.0);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("clear_color_g"), -1);
  sq_pushfloat(virtual_machine, 0.0);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("clear_color_b"), -1);
  sq_pushfloat(virtual_machine, 0.0);
  sq_newslot(virtual_machine, -3, SQBool{false});

  sq_pushstring(virtual_machine, _SC("clear_color_a"), -1);
  sq_pushfloat(virtual_machine, 0.0);
  sq_newslot(virtual_machine, -3, SQBool{false});

  // end "surge" table
  sq_newslot(virtual_machine, -3, SQFalse);

  if (SQ_FAILED(
          sqstd_dofile(virtual_machine, config_script.c_str(), false, true))) {
    log_all<log_event::error>("An error ocurred while processing {}",
                              config_script.string());
    status = false;
  } else {
    status = true;
  }

  // Restore stack frame
  sq_settop(virtual_machine, top);

  return status;
}