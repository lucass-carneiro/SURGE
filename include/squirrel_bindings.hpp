/**
 * squirrel_bindings.hpp
 *
 * bindings for controlling the squirrel VM
 */

#ifndef SURGE_SQUIRREL_BINDINGS_HPP
#define SURGE_SQUIRREL_BINDINGS_HPP

#include "log.hpp"
#include "safe_ops.hpp"

// clang-format off
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdio.h>
// clang-format on

#include <filesystem>
#include <mutex>
#include <optional>
#include <tl/expected.hpp>

#ifdef _MSC_VER
#pragma comment(lib, "squirrel.lib")
#pragma comment(lib, "sqstdlib.lib")
#endif

#ifdef SQUNICODE
#define scvprintf vwprintf
#else
#define scvprintf vprintf // NOLINT(cppcoreguidelines-macro-usage)
#endif

namespace surge {

/**
 * Concept that represents retrievable squirrel types
 */
template <typename T>
concept retrivable_vm_type =
    std::is_same<T, SQInteger>::value || std::is_same<T, SQFloat>::value ||
    std::is_same<T, SQBool>::value || std::is_same<T, const SQChar *>::value;

/**
 * Manages a squirrel VM instance.
 */
class squirrel_vm {
public:
  squirrel_vm(SQInteger stack_size) noexcept;
  ~squirrel_vm() noexcept;

  /**
   * Calls a VM defined function inside the "surge" table, that is, a VM
   * callback
   *
   * @param v The squirrel VM.
   * @param The name of the function to call.
   */
  auto call_callback(const SQChar *function_name) noexcept -> bool;

  /**
   * Pushes the engine core context into the virtual machine
   *
   * @param v The squirrel VM
   * @param s
   */
  auto load_context(const std::filesystem::path &config_script) noexcept
      -> bool;

  /**
   * Retrieves a value from the surge root table
   *
   * @param v The squirrel VM.
   * @param param_name The name of the config parameter to retrieve.
   */
  template <retrivable_vm_type T>
  auto surge_retrieve(const SQChar *param_name) noexcept -> std::optional<T> {

    // Save stack size to restore the stack state after the call
    const auto stack_top = sq_gettop(virtual_machine);

    // get the surge table
    sq_pushroottable(virtual_machine);
    sq_pushstring(virtual_machine, _SC("surge"), -1);
    sq_get(virtual_machine, -2);

    // Try to retrieve the configuration parameter
    std::optional<T> result;

    sq_pushstring(virtual_machine, param_name, -1);

    if (SQ_SUCCEEDED(sq_get(virtual_machine, -2))) {
      T buffer{0};

      // Create result based on requested type
      if constexpr (std::is_same<T, SQInteger>::value) {

        if (SQ_SUCCEEDED(sq_getinteger(virtual_machine, -1, &buffer))) {
          result = buffer;
        } else {
          log_all<log_event::error>(
              "Unable to retrieve integer value surge.{}.", param_name);
          result = std::optional<T>{};
        }

      } else if constexpr (std::is_same<T, SQFloat>::value) {

        if (SQ_SUCCEEDED(sq_getfloat(virtual_machine, -1, &buffer))) {
          result = buffer;
        } else {
          log_all<log_event::error>("Unable to retrieve float value surge.{}.",
                                    param_name);
          result = std::optional<T>{};
        }

      } else if constexpr (std::is_same<T, SQBool>::value) {

        if (SQ_SUCCEEDED(sq_getbool(virtual_machine, -1, &buffer))) {
          result = buffer;
        } else {
          log_all<log_event::error>(
              "Unable to retrieve boolean value surge.{}.", param_name);
          result = std::optional<T>{};
        }

      } else if constexpr (std::is_same<T, const SQChar *>::value) {

        if (SQ_SUCCEEDED(sq_getstring(virtual_machine, -1, &buffer))) {
          result = buffer;
        } else {
          log_all<log_event::error>("Unable to retrieve string value surge.{}.",
                                    param_name);
          result = std::optional<T>{};
        }
      }
    } else {
      log_all<log_event::error>("Unable to find {} in the surge root table.",
                                param_name);
      result = std::optional<T>{};
    }

    // restores the original stack size
    sq_settop(virtual_machine, stack_top);

    return result;
  }

  /**
   * Retrieves a value from the surge root table and safelly casts it to target
   * type
   *
   * @param v The squirrel VM.
   * @param param_name The name of the config parameter to retrieve.
   */
  template <retrivable_vm_type T, std::integral target_type>
  auto surge_retrieve(const SQChar *param_name) noexcept
      -> tl::expected<target_type, cast_error> {

    using namespace tl;

    const auto retrieve_result = surge_retrieve<T>(param_name);

    if (!retrieve_result.has_value()) {
      return unexpected(cast_error::missing_input_type);
    } else {
      return safe_cast<target_type>(retrieve_result.value());
    }
  }

  squirrel_vm(const squirrel_vm &) = delete;
  squirrel_vm(squirrel_vm &&) = delete;
  auto operator=(squirrel_vm &) -> squirrel_vm & = delete;
  auto operator=(squirrel_vm &&) -> squirrel_vm & = delete;

private:
  SQInteger initial_stack_size = 1024;
  HSQUIRRELVM virtual_machine = nullptr;
  std::mutex vm_mutex;
};

/**
 * The function invoked by the VM to print.
 *
 * @param v The squirrel VM
 * @param s The string formater.
 */
void squirrel_print_function(HSQUIRRELVM v, const SQChar *s, ...) noexcept;

/**
 * The function invoked by the VM to print errors.
 *
 * @param v The squirrel VM
 * @param s The string formater.
 */
void squirrel_error_function(HSQUIRRELVM v, const SQChar *s, ...) noexcept;

class global_squirrel_vm {
public:
  static auto get() -> squirrel_vm & {
    static squirrel_vm vm(stack_size);
    return vm;
  }

  static const SQInteger stack_size;

  global_squirrel_vm(const global_squirrel_vm &) = delete;
  global_squirrel_vm(global_squirrel_vm &&) = delete;

  auto operator=(global_squirrel_vm) -> global_squirrel_vm & = delete;

  auto operator=(const global_squirrel_vm &) -> global_squirrel_vm & = delete;

  auto operator=(global_squirrel_vm &&) -> global_squirrel_vm & = delete;

  ~global_squirrel_vm() = default;

private:
  global_squirrel_vm() = default;
};

} // namespace surge

#endif // SURGE_SQUIRREL_BINDINGS_HPP