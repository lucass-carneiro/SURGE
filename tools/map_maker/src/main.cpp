// clang-format off
#include "depth_data_file.hpp"

#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfRgbaFile.h>

#include <docopt/docopt.h>
#include <fmt/core.h>

#include <exception>
#include <vector>
#include <cstdio>

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
static void extract_depth(const std::string &input, const std::string &output) {
  fmt::println("Extracting depth data from {}", input);

  Imf::RgbaInputFile file(input.c_str());

  const auto dw{file.dataWindow()};
  const auto width{dw.max.x - dw.min.x + 1};
  const auto height{dw.max.y - dw.min.y + 1};

  Imf::Array2D<Imf::Rgba> pixels;
  pixels.resizeErase(height, width);

  file.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * width, 1,
                      static_cast<std::size_t>(width));
  file.readPixels(dw.min.y, dw.max.y);

  fmt::println("Repacking depth data");
  const auto u_width{static_cast<std::uint32_t>(width)};
  const auto u_height{static_cast<std::uint32_t>(height)};

  const auto s_width{static_cast<std::size_t>(width)};
  const auto s_height{static_cast<std::size_t>(height)};
  const std::size_t total_size{s_width * s_height};

  std::vector<float> data;
  data.reserve(total_size);

  for (std::uint32_t y = 0; y < u_height; y++) {
    for (std::uint32_t x = 0; x < u_width; x++) {
      const auto color{float{pixels[static_cast<long>(y)][static_cast<long>(x)].r}};
      const auto alpha{float{pixels[static_cast<long>(y)][static_cast<long>(x)].a}};
      const auto final_value{color * alpha};
      const auto I{s_width * y + x};
      data[I] = final_value;
    }
  }

  fmt::println("Saving repacked data");

  using std::fopen, std::fwrite, std::fclose;

  auto out_file{fopen(output.c_str(), "wb")};

  fwrite(surge::files::depth_file::header, surge::files::depth_file::header_size, 1, out_file);
  fwrite(&u_width, sizeof(u_width), 1, out_file);
  fwrite(&u_height, sizeof(u_height), 1, out_file);
  fwrite(&total_size, sizeof(total_size), 1, out_file);
  fwrite(data.data(), sizeof(float), total_size, out_file);

  fclose(out_file);
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