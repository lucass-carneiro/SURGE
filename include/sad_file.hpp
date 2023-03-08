#ifndef SURGE_SAD_FILE_HPP
#define SURGE_SAD_FILE_HPP

#include "file.hpp"

#include <cstdint>
#include <string_view>

namespace surge {

struct animation_data {
  std::uint32_t index{0};
  std::uint32_t x{0};
  std::uint32_t y{0};
  std::uint32_t Sw{0};
  std::uint32_t Sh{0};
  std::uint32_t rows{0};
  std::uint32_t cols{0};
};

auto load_sad_file(const std::filesystem::path &p) noexcept -> load_file_return_t;

auto get_animation(const load_file_span &file_span, std::uint32_t index,
                   bool bound_check = true) noexcept -> std::optional<animation_data>;

} // namespace surge

#endif