/**
 * cli.hpp
 *
 * Handles command line interface opperations
 */

#ifndef SURGE_CLI_HPP
#define SURGE_CLI_HPP

#include "file.hpp"
#include "safe_ops.hpp"

// clang-format off
#include <docopt/docopt.h>
#include <tl/expected.hpp>
// clang-format on

#include <filesystem>

namespace surge {

/**
 * The types of errors that docopt can throw
 */
enum class docopt_error_type : int {
  docopt_exit_help,
  docopt_exit_version,
  docopt_language_error,
  docopt_argument_error,
  docopt_unhandled_error
};

/**
 * @brief Draws the SURGE logo on the terminal
 *
 */
void draw_logo() noexcept;

/**
 * Parse command line arguments and return them.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The argument strings passed to the program.
 * @return The parsed command line options or an error in case of failure.
 */
auto parse_arguments(int argc, char **argv) noexcept
    -> tl::expected<docopt::Options, docopt_error_type>;

auto get_arg_string(const docopt::Options &opts, const char *arg) noexcept
    -> std::optional<const char *>;

auto get_arg_long(const docopt::Options &opts, const char *arg) noexcept -> std::optional<long>;

/**
 * TODO: doc
 */
auto get_file_path(const docopt::Options &opts, const char *arg, const char *ext) noexcept
    -> std::optional<std::filesystem::path>;

} // namespace surge

#endif // SURGE_CLI_HPP