#include "cli.hpp"

#include "file.hpp"
#include "log.hpp"
#include "options.hpp"

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

#include <cstdio>

/**
 * The program help/usage message that is also used to generate the command line
 * parser.
 */
static constexpr const char *USAGE =
    R"(SURGE engine.

    Usage:
      surge [options] <config-script> <startup-script>
      surge (-h | --help)
      surge --version

    Options:
      -h --help                                 Show this screen.
      --version                                 Show version.
      --num-threads=<num>                       The total number of threads to use. If negative, use all available threads. [default: 2]
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

void surge::draw_logo() noexcept {
#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

#ifdef SURGE_USE_LOG_COLOR
  std::printf("\033[1;38;2;220;20;60m%s\033[0m", LOGO);
#else
  std::printf("%s", LOGO);
#endif
}

auto surge::parse_arguments(int argc, char **argv) noexcept -> std::optional<cmd_opts> {
#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

  try {
    auto cmd_line_args
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        = docopt::docopt_parse(USAGE, {argv + 1, argv + argc}, true, true, false);

    return cmd_line_args;

  } catch (const docopt::DocoptExitHelp &) {
    std::printf("%s\n", USAGE);
    return {};

  } catch (const docopt::DocoptExitVersion &) {
    std::printf("%s\n", VERSION_STRING);
    return {};

  } catch (const docopt::DocoptLanguageError &) {
    log_error("Internal problem: a syntax error ocurred in the "
              "USAGE string. Please contact a "
              "developper");
    return {};

  } catch (const docopt::DocoptArgumentError &) {
    log_info("Unrecognized arguments passed. Rerun with the --help option "
             "for usage instructions.");
    return {};

  } catch (const std::exception &error) {
    log_error("Unhandled exception while running Docopt {}", error.what());
    return {};
  }
}

auto surge::get_arg_string(const cmd_opts &opts, const char *arg) noexcept
    -> std::optional<const char *> {
  try {
    return opts.at(arg).asString().c_str();
  } catch (const std::exception &error) {
    log_error("Unable to interpret the command line argument {} as a string", arg);
    return {};
  }
}

auto surge::get_arg_long(const cmd_opts &opts, const char *arg) noexcept -> std::optional<long> {
#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

  try {
    return opts.at(arg).asLong();
  } catch (const std::exception &error) {
    log_error("Unable to interpret the command line argument {} as a long", arg);
    return {};
  }
}

auto surge::get_file_path(const cmd_opts &opts, const char *arg, const char *ext) noexcept
    -> std::optional<std::filesystem::path> {
#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

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