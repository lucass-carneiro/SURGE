#include "tinyexr.hpp"

#include <docopt/docopt.h>
#include <exception>
#include <fmt/core.h>

// clang-format off
static const auto usage_string{
    R"(SURGE Map Maker.

    Usage:
      map_maker extract-depth <openEXR-depth-map> <output-file>
      map_maker (-h | --help)
      map_maker --version

    Options:
      -h --help     Show this screen.
      --version     Show version.
)"};
// clang-format on

// NOLINTNEXTLINE
static void extract_depth(const std::string &input, const std::string &) {
  fmt::println("Extracting depth data from {}", input);

  float *out{nullptr}; // width * height * RGBA
  int width{0};
  int height{0};
  const char *err{nullptr};

  const auto result{LoadEXR(&out, &width, &height, input.c_str(), &err)};

  if (result != TINYEXR_SUCCESS) {
    if (err) {
      fmt::println("TinyEXR error: {}", err);
      FreeEXRErrorMessage(err); // release memory of error message.
    }
  }

  fmt::println("{} read sucesfully. Dimensions: {}x{}", input, width, height);
  fmt::println("{:.16f}", out[0]);
  fmt::println("{:.16f}", out[1]);

  free(out); // NOLINT
}

auto main(int argc, const char **argv) noexcept -> int {
  using namespace docopt;

  try {
    const std::vector<std::string> args{argv + 1, argv + argc}; // NOLINT
    const auto parsed_args{docopt_parse(usage_string, args, true, true)};

    if (parsed_args.at("extract-depth")) {
      extract_depth(parsed_args.at("<openEXR-depth-map>").asString(),
                    parsed_args.at("<output-file>").asString());
    }

    return EXIT_SUCCESS;

  } catch (DocoptLanguageError &e) {
    fmt::println("Internal docopt error: {}", e.what());
    return EXIT_FAILURE;
  } catch (DocoptArgumentError &) {
    fmt::print("Unrecognized usage pattern\n{}", usage_string);
    return EXIT_FAILURE;
  } catch (DocoptExitHelp &) {
    fmt::print("{}", usage_string);
    return EXIT_FAILURE;
  } catch (DocoptExitVersion &e) {
    fmt::println("SURGE map maker tool v1.0.0");
    return EXIT_SUCCESS;
  } catch (std::exception &e) {
    fmt::println("Error: {}", e.what());
    return EXIT_FAILURE;
  }
}