#ifndef SURGE_DEPTH_DATA_FILE_HPP
#define SURGE_DEPTH_DATA_FILE_HPP

#include <cstdint>

namespace surge::files {

#define FILE_HEADER_STRING "SURGE depth data file v1.0.0"

struct depth_file {
  static constexpr auto header{FILE_HEADER_STRING};
  static constexpr std::size_t header_size{sizeof(FILE_HEADER_STRING)};
  std::uint32_t width{0};
  std::uint32_t height{0};
  std::size_t total_size{0};
  float *data{nullptr}; // Row major ordering
};

#undef FILE_HEADER_STRING

} // namespace surge::files

#endif // SURGE_DEPTH_DATA_FILE_HPP