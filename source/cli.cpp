#include "cli.hpp"

#include "file.hpp"
#include "logging_system/logging_system.hpp"
#include "options.hpp"

#include <cstdio>
#include <cstring>

static constexpr const char *USAGE =
    R"(SURGE engine.

    Usage:
      surge <config-script> <startup-script>
      surge (-h | --help)
      surge --version

    Options:
      -h --help           Show this screen.
      --version           Show version.
)";

static constexpr const char *VERSION_STRING
    = "SURGE v" SURGE_VERSION_MAJOR_STRING "." SURGE_VERSION_MINOR_STRING
      "." SURGE_VERSION_PATCH_STRING;

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOGO \
  "   d888888o.   8 8888      88 8 888888888o.        ,o888888o.    8 8888888888  \n" \
  " .`8888:' `88. 8 8888      88 8 8888    `88.      8888     `88.  8 8888        \n" \
  " 8.`8888.   Y8 8 8888      88 8 8888     `88   ,8 8888       `8. 8 8888        \n" \
  " `8.`8888.     8 8888      88 8 8888     ,88   88 8888           8 8888        \n" \
  "  `8.`8888.    8 8888      88 8 8888.   ,88'   88 8888           8 888888888888\n" \
  "   `8.`8888.   8 8888      88 8 888888888P'    88 8888           8 8888        \n" \
  "    `8.`8888.  8 8888      88 8 8888`8b        88 8888   8888888 8 8888        \n" \
  "8b   `8.`8888. ` 8888     ,8P 8 8888 `8b.      `8 8888       .8' 8 8888        \n" \
  "`8b.  ;8.`8888   8888   ,d8P  8 8888   `8b.       8888     ,88'  8 8888        \n" \
  " `Y8888P ,88P'    `Y88888P'   8 8888     `88.      `8888888P'    8 888888888888\n"
// clang-format on

void surge::draw_logo() noexcept {
  using std::printf;
#ifdef SURGE_USE_LOG_COLOR
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::printf("\033[1;38;2;220;20;60m" LOGO "\033[0m");
#else
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::printf(LOGO);
#endif
}

void print_help() noexcept {
  using std::printf;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("%s\n", USAGE);
}

void print_version() noexcept {
  using std::printf;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("%s\n", VERSION_STRING);
}

auto surge::parse_arguments(int argc, char **argv) noexcept -> cmd_args_t {
  using std::strcmp;

  if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    print_help();
    return {};
  } else if (argc == 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
    print_version();
    return {};
  } else if (argc == 3) {
    if (validate_path(argv[1], ".lua") && validate_path(argv[2], ".lua")) {
      return {std::make_tuple(argv[1], argv[2])};
    } else {
      return {};
    }
  } else {
    print_help();
    return {};
  }
}