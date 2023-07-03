#include "image_loader.hpp"

#include "allocator.hpp"
#include "file.hpp"
#include "logging_system/logging_system.hpp"
#include "options.hpp"

auto surge::load_image(const char *p, const char *ext) noexcept -> std::optional<image> {
  log_info("Loading image file {}", p);

  auto file{load_file(p, ext, false)};
  if (!file) {
    log_error("Unable to load image file {}", p);
    return {};
  }

  int x{0}, y{0}, channels_in_file{0};
  auto img_data{
      stbi_load_from_memory(static_cast<stbi_uc *>(static_cast<void *>(file.value().data())),
                            file.value().size(), &x, &y, &channels_in_file, 0)};

  if (img_data == nullptr) {
    log_error("Unable to load image file {} due to stbi error: {}", p, stbi_failure_reason());
    mi_free(file.value().data());
    return {};
  }

  mi_free(file.value().data());
  return image{.width = x, .height = y, .channels_in_file = channels_in_file, .data = img_data};
}