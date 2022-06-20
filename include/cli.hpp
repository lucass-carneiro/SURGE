/**
 * cli.hpp
 *
 * Handles command line interface opperations
 */

#ifndef SURGE_CLI_HPP
#define SURGE_CLI_HPP

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

enum class config_file_error_type : int {
  file_does_not_exist,
  file_is_not_regular,
  file_is_not_nut,
  unknow_exception
};

/**
 * Parse command line arguments and return them.
 *
 * @param argc The number of arguments passed to the program.
 * @param argv The argument strings passed to the program.
 * @return The parsed command line options or an error in case of failure.
 */
auto parse_arguments(int argc, char **argv) noexcept
    -> tl::expected<docopt::Options, docopt_error_type>;

/**
 * Validates the path of the passed configuration script.
 *
 * @param opts Command line options passed from docopt.
 * @return True for a valid path, false for an invalid path.
 */
auto validate_config_script_path(const docopt::Options &opts) noexcept
    -> tl::expected<std::filesystem::path, config_file_error_type>;

} // namespace surge

#endif // SURGE_CLI_HPP