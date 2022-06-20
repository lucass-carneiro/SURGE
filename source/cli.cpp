#include "cli.hpp"

#include "log.hpp"
#include "options.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <tl/expected.hpp>

/**
 * The program help/usage message that is also used to generate the command line
 * parser.
 */
static constexpr const char *USAGE =
    R"(SURGE engine.

    Usage:
      surge [--config-file=<path>] <config-script>
      surge (-h | --help)
      surge --version

    Options:
      -h --help          Show this screen.
      --version          Show version.
      --config-file=<ext>  Path to an engine config file. [default: "surge_config.nut"]
)";

static constexpr const char *VERSION_STRING =
    "SURGE v" SURGE_VERSION_MAJOR_STRING "." SURGE_VERSION_MINOR_STRING
    "." SURGE_VERSION_PATCH_STRING;

// clang-format off
static constexpr const char *LOGO =
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

auto surge::parse_arguments(int argc, char **argv) noexcept
    -> tl::expected<docopt::Options, docopt_error_type> {
  using tl::unexpected;

  try {
    auto cmd_line_args
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        = docopt::docopt_parse(USAGE, {argv + 1, argv + argc}, true, true,
                               false);

    log_all<log_event::logo>(LOGO);

    return cmd_line_args;

  } catch (const docopt::DocoptExitHelp &) {
    std::cout << USAGE << std::endl;
    return unexpected(docopt_error_type::docopt_exit_help);

  } catch (const docopt::DocoptExitVersion &) {
    log_all<log_event::logo>(LOGO);
    std::cout << VERSION_STRING << std::endl;
    return unexpected(docopt_error_type::docopt_exit_version);

  } catch (const docopt::DocoptLanguageError &) {
    log_all<log_event::error>("Internal problem: a syntax error ocurred in the "
                              "USAGE string. Please contact a "
                              "developper");
    return unexpected(docopt_error_type::docopt_language_error);

  } catch (const docopt::DocoptArgumentError &) {
    log_all<log_event::message>(
        "Unrecognized arguments passed. Rerun with the --help option "
        "for usage instructions.");
    return unexpected(docopt_error_type::docopt_argument_error);

  } catch (const std::exception &error) {
    log_all<log_event::error>("Unhandled exception while running Docopt {}",
                              error.what());
    return unexpected(docopt_error_type::docopt_unhandled_error);
  }
}

auto surge::validate_config_script_path(const docopt::Options &opts) noexcept
    -> tl::expected<std::filesystem::path, config_file_error_type> {

  using tl::unexpected;

  try {
    std::filesystem::path candidate_path(opts.at("<config-script>").asString());

    if (!std::filesystem::exists(candidate_path)) {
      log_all<log_event::error>(
          "The configuration script path {} does not exist.",
          candidate_path.string());
      return unexpected(surge::config_file_error_type::file_does_not_exist);
    }

    if (!std::filesystem::is_regular_file(candidate_path)) {
      log_all<log_event::error>(
          "The configuration script path {} does not point to a regular file.",
          candidate_path.string());
      return unexpected(surge::config_file_error_type::file_is_not_regular);
    }

    if (candidate_path.extension() != ".nut") {
      log_all<log_event::error>(
          "The configuration script path {} does not point to a .nut file. {}",
          candidate_path.string());
      return unexpected(surge::config_file_error_type::file_is_not_nut);
    }

    return candidate_path;

  } catch (const std::exception &e) {
    std::cout << "Error while validating configuration file path : " << e.what()
              << std::endl;

    return unexpected(surge::config_file_error_type::unknow_exception);
  }
}