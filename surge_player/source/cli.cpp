#include "cli.hpp"

#include "files.hpp"
#include "options.hpp"

#include <cstdio>
#include <cstring>

// clang-format off
static constexpr const char *USAGE =
"SURGE engine.                          \n\n"
"  Usage:                                 \n"
"    surge <config-file>                  \n"
"    surge (-h | --help)                  \n"
"    surge --version                    \n\n"
"  Options:                               \n"
"    -h --help           Show this screen.\n"
"    --version           Show version.    \n";
// clang-format on

static constexpr const char *VERSION_STRING
    = "SURGE v" SURGE_VERSION_MAJOR_STRING "." SURGE_VERSION_MINOR_STRING
      "." SURGE_VERSION_PATCH_STRING "\n";

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

void surge::cli::draw_logo() noexcept {
  using std::printf;
#ifdef SURGE_USE_LOG_COLOR
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::printf("\033[1;38;2;220;20;60m%s\033[m", LOGO);
#else
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  std::printf("%s", LOGO);
#endif
}

void print_help() noexcept {
  using std::printf;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("%s", USAGE);
}

void print_version() noexcept {
  using std::printf;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("%s", VERSION_STRING);
}

auto surge::cli::parse_arguments(int argc, char **argv) noexcept -> bool {
  using std::strcmp;

  if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    print_help();
    return false;
  } else if (argc == 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
    print_version();
    return false;
  } else if (argc == 2) {
    return surge::files::validate_path(argv[1]);
  } else {
    print_help();
    return {};
  }
}