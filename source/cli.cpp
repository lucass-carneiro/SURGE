#include "cli.hpp"

#include "log.hpp"
#include "options.hpp"
#include "safe_ops.hpp"

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

void surge::draw_logo() noexcept { log_all<log_event::logo>(LOGO); }

auto surge::parse_arguments(int argc, char **argv) noexcept
    -> tl::expected<docopt::Options, docopt_error_type> {
  using tl::unexpected;

  try {
    auto cmd_line_args
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        = docopt::docopt_parse(USAGE, {argv + 1, argv + argc}, true, true,
                               false);

    return cmd_line_args;

  } catch (const docopt::DocoptExitHelp &) {
    std::cout << USAGE << std::endl;
    return unexpected(docopt_error_type::docopt_exit_help);

  } catch (const docopt::DocoptExitVersion &) {
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
    -> tl::expected<std::filesystem::path, path_error_type> {

  using tl::unexpected;

  std::filesystem::path candidate_path(opts.at("<config-script>").asString());
  const auto validate_result = validate_path(candidate_path, ".nut");

  if (validate_result.has_value()) {
    return unexpected(validate_result.value());
  } else {
    return candidate_path;
  }
}