#ifndef SURGE_SAD_FILE_HPP
#define SURGE_SAD_FILE_HPP

#include "file.hpp"

#include <cstdint>
#include <cstring>

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

constexpr const std::string_view sad_file_id_string{"SAD - SURGE Animation Data File"};
constexpr const std::size_t sad_file_id_string_size{sad_file_id_string.size() + 1};
constexpr const std::size_t sad_file_header_size{sad_file_id_string_size + sizeof(std::uint32_t)};
constexpr const std::size_t sad_file_animation_data_size{sizeof(animation_data)};

template <surge_allocator alloc_t>
inline auto load_sad_file(alloc_t *allocator, const std::filesystem::path &p) noexcept
    -> load_file_return_t {

  const auto file_data{load_file(allocator, p, ".sad")};

  // Check if the IO operation succeeded
  if (!file_data) {
    glog<log_event::error>("Unable to load sad file {}.", p.c_str());
    return {};
  }

  // Check if the size is plausible
  if (file_data.value().size() < sad_file_header_size) {
    glog<log_event::error>("The file {} is too short to contain valid animation data.", p.c_str());

    allocator->free(file_data.value().data());
    return {};
  }

  // Check the identification header
  const auto header{static_cast<const char *>(
      static_cast<void *>(file_data.value().subspan(0, sad_file_id_string_size).data()))};
  if (std::strcmp(header, sad_file_id_string.data()) != 0) {
    glog<log_event::error>("The file {} does not contain a SAD file header.", p.c_str());

    allocator->free(file_data.value().data());
    return {};
  }

  // Get the number of stored animations
  const auto animation_count{*static_cast<std::uint32_t *>(static_cast<void *>(
      file_data.value().subspan(sad_file_id_string_size, sizeof(std::uint32_t)).data()))};

  // Check the total file size
  if (file_data.value().size()
      != (animation_count * sad_file_animation_data_size + sad_file_header_size)) {
    glog<log_event::error>("The SAD file {} cannot store {} animations. {}", p.c_str(),
                           animation_count, file_data.value().size());

    allocator->free(file_data.value().data());
    return {};
  }

  glog<log_event::message>("Loaded {} with {} animations.", p.c_str(), animation_count);
  return file_data;
}

template <bool bound_check = true>
inline auto get_animation(const load_file_span &file_span, std::uint32_t index) noexcept
    -> std::optional<animation_data> {

  if constexpr (bound_check) {
    const auto animation_count{*static_cast<std::uint32_t *>(static_cast<void *>(
        file_span.subspan(sad_file_id_string_size, sizeof(std::uint32_t)).data()))};

    if (index >= animation_count) {
      glog<log_event::error>(
          "Unable to retrieve anination index {}. SAD file contains only {} animations", index,
          animation_count);
      return {};
    }
  }

  const std::size_t animation_start_idx{sad_file_header_size
                                        + index * sad_file_animation_data_size};
  const animation_data data{*static_cast<animation_data *>(static_cast<void *>(
      file_span.subspan(animation_start_idx, sad_file_animation_data_size).data()))};

  return data;
}

} // namespace surge

#endif