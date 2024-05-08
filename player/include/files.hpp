#ifndef SURGE_FILES_HPP
#define SURGE_FILES_HPP

#include "container_types.hpp"
#include "error_types.hpp"
#include "tasks.hpp"

#include <tl/expected.hpp>

namespace surge::files {

using file_size_t = std::uintmax_t;
using file_data_t = vector<std::byte>;
using file = tl::expected<file_data_t, error>;

auto validate_path(const char *path) noexcept -> bool;

auto load_file(const char *path, bool append_null_byte) noexcept -> file;

struct image_data {
  int width;
  int height;
  int channels;
  unsigned char *pixels;
  const char *file_name;
};

struct openEXR_image_data {
  int width;
  int height;
  void *pixels;
  const char *file_name;
};

using image = tl::expected<image_data, error>;
using openEXR_image = tl::expected<openEXR_image_data, error>;
using img_future = std::future<image>;

auto load_image(const char *path, bool flip = true) noexcept -> image;
auto load_image_task(const char *path, bool flip = true) noexcept -> img_future;
void free_image(image_data &) noexcept;
void free_image_task(image_data &) noexcept;

auto load_openEXR(const char *path) noexcept -> openEXR_image;
void free_openEXR(openEXR_image_data &data) noexcept;

} // namespace surge::files

#endif // SURGE_FILES_HPP