#include "cli.hpp"

#include "file.hpp"
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
      surge [--pages=<num>] [--num-threads=<num>] [--thread-mem-quota=<quota>] <config-script> <startup-script>
      surge (-h | --help)
      surge --version

    Options:
      -h --help                   Show this screen.
      --version                   Show version.
      --config-script=<path>      Path to an engine config file. [default: "config.lua"]
      --pages=<num>               The max. amount of engine memory to use (in page size multiples). [default: 524288]
      --num-threads=<num>         The total number of threads to use. If negative, use all available threads. [default: 4]
      --thread-mem-quota=<quota>  The percentage of the total memory to be assigned to the worker threads (in bytes). [default: 75]
)";

static constexpr const char *VERSION_STRING
    = "SURGE v" SURGE_VERSION_MAJOR_STRING "." SURGE_VERSION_MINOR_STRING
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

void surge::draw_logo() noexcept { glog<log_event::logo>(LOGO); }

auto surge::parse_arguments(int argc, char **argv) noexcept
    -> tl::expected<cmd_opts, docopt_error_type> {
  using tl::unexpected;

  try {
    auto cmd_line_args
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        = docopt::docopt_parse(USAGE, {argv + 1, argv + argc}, true, true, false);

    return cmd_line_args;

  } catch (const docopt::DocoptExitHelp &) {
    std::cout << USAGE << std::endl;
    return unexpected(docopt_error_type::docopt_exit_help);

  } catch (const docopt::DocoptExitVersion &) {
    std::cout << VERSION_STRING << std::endl;
    return unexpected(docopt_error_type::docopt_exit_version);

  } catch (const docopt::DocoptLanguageError &) {
    glog<log_event::error>("Internal problem: a syntax error ocurred in the "
                           "USAGE string. Please contact a "
                           "developper");
    return unexpected(docopt_error_type::docopt_language_error);

  } catch (const docopt::DocoptArgumentError &) {
    glog<log_event::message>("Unrecognized arguments passed. Rerun with the --help option "
                             "for usage instructions.");
    return unexpected(docopt_error_type::docopt_argument_error);

  } catch (const std::exception &error) {
    glog<log_event::error>("Unhandled exception while running Docopt {}", error.what());
    return unexpected(docopt_error_type::docopt_unhandled_error);
  }
}

auto surge::get_arg_string(const cmd_opts &opts, const char *arg) noexcept
    -> std::optional<const char *> {
  try {
    return opts.at(arg).asString().c_str();
  } catch (const std::exception &error) {
    glog<log_event::error>("Unable to interpret the command line argument {} as a string", arg);
    return {};
  }
}

auto surge::get_arg_long(const cmd_opts &opts, const char *arg) noexcept -> std::optional<long> {
  try {
    return opts.at(arg).asLong();
  } catch (const std::exception &error) {
    glog<log_event::error>("Unable to interpret the command line argument {} as a long", arg);
    return {};
  }
}

auto surge::get_file_path(const cmd_opts &opts, const char *arg, const char *ext) noexcept
    -> std::optional<std::filesystem::path> {

  const auto arg_string{get_arg_string(opts, arg)};
  if (!arg_string.has_value()) {
    return {};
  }

  std::filesystem::path candidate_path(arg_string.value());

  const auto validate_result{validate_path(candidate_path, ext)};
  if (validate_result == true) {
    return candidate_path;
  } else {
    return {};
  }
}