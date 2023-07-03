#include "sad_file.hpp"

#include "allocator.hpp"
#include "logging_system/logging_system.hpp"
#include "options.hpp"

#include <cstring>

struct animation_data {
  std::uint32_t index{0};
  std::uint32_t x{0};
  std::uint32_t y{0};
  std::uint32_t Sw{0};
  std::uint32_t Sh{0};
  std::uint32_t rows{0};
  std::uint32_t cols{0};
};

static constexpr const std::string_view sad_file_id_string{"SAD - SURGE Animation Data File"};
static constexpr const std::size_t sad_file_id_string_size{sad_file_id_string.size() + 1};
static constexpr const std::size_t sad_file_header_size{sad_file_id_string_size
                                                        + sizeof(std::uint32_t)};
static constexpr const std::size_t sad_file_animation_data_size{sizeof(animation_data)};

auto surge::load_sad_file(const char *p) noexcept -> std::optional<sad_file_contents> {

  const auto file_data{load_file(p, ".sad", false)};

  // Check if the IO operation succeeded
  if (!file_data) {
    log_error("Unable to load sad file {}.", p);
    return {};
  }

  // Check if the size is plausible
  if (file_data.value().size() < sad_file_header_size) {
#ifdef SURGE_SYSTEM_Windows
    log_error(L"The file {} is too short to contain valid animation data.", p);
#else
    log_error("The file {} is too short to contain valid animation data.", p);
#endif

    mi_free(file_data.value().data());
    return {};
  }

  // Check the identification header
  const auto header{static_cast<const char *>(
      static_cast<void *>(file_data.value().subspan(0, sad_file_id_string_size).data()))};
  if (std::strcmp(header, sad_file_id_string.data()) != 0) {
    log_error("The file {} does not contain a SAD file header.", p);

    mi_free(file_data.value().data());
    return {};
  }

  // Get the number of stored animations
  const auto animation_count{*static_cast<std::uint32_t *>(static_cast<void *>(
      file_data.value().subspan(sad_file_id_string_size, sizeof(std::uint32_t)).data()))};

  // Check the total file size
  if (file_data.value().size()
      != (animation_count * sad_file_animation_data_size + sad_file_header_size)) {
    log_error("The SAD file {} cannot store {} animations. {}", p, animation_count,
              file_data.value().size());

    mi_free(file_data.value().data());
    return {};
  }

  // Reserve memory to contain the animation data
  sad_file_contents file_contents{};
  file_contents.path = p;
  file_contents.x.reserve(animation_count);
  file_contents.y.reserve(animation_count);
  file_contents.Sw.reserve(animation_count);
  file_contents.Sh.reserve(animation_count);
  file_contents.rows.reserve(animation_count);
  file_contents.cols.reserve(animation_count);

  // Read from file to struct of arrays
  for (std::uint32_t i = 0; i < animation_count; i++) {
    const std::size_t animation_start_idx{sad_file_header_size + i * sad_file_animation_data_size};
    const animation_data data{*static_cast<animation_data *>(static_cast<void *>(
        file_data.value().subspan(animation_start_idx, sad_file_animation_data_size).data()))};
    file_contents.x.push_back(data.x);
    file_contents.y.push_back(data.y);
    file_contents.Sw.push_back(data.Sw);
    file_contents.Sh.push_back(data.Sh);
    file_contents.rows.push_back(data.rows);
    file_contents.cols.push_back(data.cols);
  }

  mi_free(file_data.value().data());
  log_info("Loaded {} with {} animations.", p, animation_count);

  return file_contents;
}