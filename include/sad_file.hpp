#ifndef SURGE_SAD_FILE_HPP
#define SURGE_SAD_FILE_HPP

#include "allocator.hpp"
#include "file.hpp"

#include <EASTL/vector.h>
#include <cstdint>
#include <string_view>

namespace surge {

struct sad_file_contents {
  const char *path;
  eastl::vector<std::uint32_t, eastl_allocator> x;
  eastl::vector<std::uint32_t, eastl_allocator> y;
  eastl::vector<std::uint32_t, eastl_allocator> Sw;
  eastl::vector<std::uint32_t, eastl_allocator> Sh;
  eastl::vector<std::uint32_t, eastl_allocator> rows;
  eastl::vector<std::uint32_t, eastl_allocator> cols;

  sad_file_contents()
      : x{eastl_allocator()},
        y{eastl_allocator()},
        Sw{eastl_allocator()},
        Sh{eastl_allocator()},
        rows{eastl_allocator()},
        cols{eastl_allocator()} {}
};

auto load_sad_file(const char *p) noexcept -> std::optional<sad_file_contents>;

} // namespace surge

#endif