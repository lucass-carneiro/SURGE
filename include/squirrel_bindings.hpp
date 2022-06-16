/**
 * squirrel_bindings.hpp
 *
 * bindings for controlling the squirrel VM
 */

#ifndef SURGE_SQUIRREL_BINDINGS_HPP
#define SURGE_SQUIRREL_BINDINGS_HPP

// clang-format off
#include "log.hpp"
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdio.h>
// clang-format on

#include <filesystem>
#include <optional>
#include <tl/expected.hpp>

#ifdef _MSC_VER
#  pragma comment(lib, "squirrel.lib")
#  pragma comment(lib, "sqstdlib.lib")
#endif

#ifdef SQUNICODE
#  define scvprintf vwprintf
#else
#  define scvprintf vprintf // NOLINT(cppcoreguidelines-macro-usage)
#endif

namespace surge {

class squirrel_vm {
public:
  squirrel_vm(SQInteger);
  ~squirrel_vm();

  auto get() -> HSQUIRRELVM { return virtual_machine; }

  squirrel_vm(const squirrel_vm &) = delete;
  squirrel_vm(squirrel_vm &&) = delete;
  auto operator=(squirrel_vm &) -> squirrel_vm & = delete;
  auto operator=(squirrel_vm &&) -> squirrel_vm & = delete;

private:
  SQInteger initial_stack_size;
  HSQUIRRELVM virtual_machine;
};

class global_squirrel_vm {
public:
  static auto instance() -> squirrel_vm & {
    static squirrel_vm vm(1024);
    return vm;
  }

  global_squirrel_vm(const global_squirrel_vm &) = delete;
  global_squirrel_vm(global_squirrel_vm &&) = delete;
  auto operator=(global_squirrel_vm &) -> global_squirrel_vm & = delete;
  auto operator=(global_squirrel_vm &&) -> global_squirrel_vm & = delete;

private:
  global_squirrel_vm() = default;
  ~global_squirrel_vm() = default;
};

/**
 * The function invoked by the VM to print.
 *
 * @param v The squirrel VM
 * @param s The string formater.
 */
void squirrel_print_function(HSQUIRRELVM v, const SQChar *s, ...);

/**
 * The function invoked by the VM to print errors.
 *
 * @param v The squirrel VM
 * @param s The string formater.
 */
void squirrel_error_function(HSQUIRRELVM v, const SQChar *s, ...);

/**
 * Calls a VM defined function inside the "surge" table, that is, a VM callback
 *
 * @param v The squirrel VM.
 * @param The name of the function to call.
 */
void call_surge_callback(HSQUIRRELVM v, const SQChar *function_name);

/**
 * Pushes the engine core context into the virtual machine
 *
 * @param v The squirrel VM
 * @param s
 */
auto push_engine_context(HSQUIRRELVM v, const std::filesystem::path &config_script) -> bool;

/**
 * Concept that represents retrievable squirrel types
 */
template <typename T>
concept retrivable_vm_type = std::is_same<T, SQInteger>::value || std::is_same<
    T, SQFloat>::value || std::is_same<T, SQBool>::value || std::is_same<T, const SQChar *>::value;

/**
 * Retrieves a window configuration option from the VM
 *
 * @param v The squirrel VM.
 * @param param_name The name of the config parameter to retrieve.
 */
template <retrivable_vm_type T> auto retrieve_window_config(HSQUIRRELVM v, const SQChar *param_name)
    -> std::optional<T> {

  // Save stack size to restore the stack state after the call
  const auto stack_top = sq_gettop(v);

  // get the surge table
  sq_pushroottable(v);
  sq_pushstring(v, _SC("surge"), -1);
  sq_get(v, -2);

  // Try to retrieve the configuration parameter
  sq_pushstring(v, param_name, -1);
  if (SQ_SUCCEEDED(sq_get(v, -2))) {
    T result{0};

    if constexpr (std::is_same<T, SQInteger>::value) {
      if (SQ_SUCCEEDED(sq_getinteger(v, -1, &result))) {
        return result;
      } else {
        log_all<log_event::error>("Unable to retrieve integer value surge.{}.", param_name);
        return {};
      }
    } else if constexpr (std::is_same<T, SQFloat>::value) {
      if (SQ_SUCCEEDED(sq_getfloat(v, -1, &result))) {
        return result;
      } else {
        log_all<log_event::error>("Unable to retrieve float value surge.{}.", param_name);
        return {};
      }
    } else if constexpr (std::is_same<T, SQBool>::value) {
      if (SQ_SUCCEEDED(sq_getbool(v, -1, &result))) {
        return result;
      } else {
        log_all<log_event::error>("Unable to retrieve boolean value surge.{}.", param_name);
        return {};
      }
    } else if constexpr (std::is_same<T, const SQChar *>::value) {
      if (SQ_SUCCEEDED(sq_getstring(v, -1, &result))) {
        return result;
      } else {
        log_all<log_event::error>("Unable to retrieve string value surge.{}.", param_name);
        return {};
      }
    }
  } else {
    log_all<log_event::error>("Unable to retrieve the configuration variable surge.{}.",
                              param_name);
    return {};
  }

  // restores the original stack size
  sq_settop(v, stack_top);
}

} // namespace surge

#endif // SURGE_SQUIRREL_BINDINGS_HPP