#ifndef SURGE_SAD_FILE_HPP
#define SURGE_SAD_FILE_HPP

#include "allocator.hpp"
#include "file.hpp"

#include <EASTL/vector.h>
#include <cstdint>
#include <string_view>

namespace surge {

struct sad_file_contents {
  std::filesystem::path path;
  eastl::vector<std::uint32_t, eastl_allocator> x;
  eastl::vector<std::uint32_t, eastl_allocator> y;
  eastl::vector<std::uint32_t, eastl_allocator> Sw;
  eastl::vector<std::uint32_t, eastl_allocator> Sh;
  eastl::vector<std::uint32_t, eastl_allocator> rows;
  eastl::vector<std::uint32_t, eastl_allocator> cols;

  sad_file_contents()
      : x{mimalloc_eastl_allocator::get()},
        y{mimalloc_eastl_allocator::get()},
        Sw{mimalloc_eastl_allocator::get()},
        Sh{mimalloc_eastl_allocator::get()},
        rows{mimalloc_eastl_allocator::get()},
        cols{mimalloc_eastl_allocator::get()} {}
};

auto load_sad_file(const std::filesystem::path &p) noexcept -> std::optional<sad_file_contents>;

} // namespace surge

#endif