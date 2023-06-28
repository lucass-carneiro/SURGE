#include "image_loader.hpp"

#include "allocator.hpp"
#include "file.hpp"
#include "logging_system/logging_system.hpp"
#include "options.hpp"

auto surge::load_image(const std::filesystem::path &p, const char *ext) noexcept
    -> std::optional<image> {
#ifdef SURGE_SYSTEM_Windows
  log_info(L"Loading image file {}", p.c_str());
#else
  log_info("Loading image file {}", p.c_str());
#endif

  auto file{load_file(p, ext, false)};
  if (!file) {
#ifdef SURGE_SYSTEM_Windows
    log_error(L"Unable to load image file {}", p.c_str());
#else
    log_error("Unable to load image file {}", p.c_str());
#endif
    return {};
  }

  int x{0}, y{0}, channels_in_file{0};
  auto img_data{
      stbi_load_from_memory(static_cast<stbi_uc *>(static_cast<void *>(file.value().data())),
                            file.value().size(), &x, &y, &channels_in_file, 0)};

  if (img_data == nullptr) {
#ifdef SURGE_SYSTEM_Windows
    log_error(L"Unable to load image file {} due to stbi error.", p.c_str());
    log_error("stbi error: {}", stbi_failure_reason());
#else
    log_error("Unable to load image file {} due to stbi error: {}", p.c_str(),
              stbi_failure_reason());
#endif
    mi_free(file.value().data());
    return {};
  }

  mi_free(file.value().data());
  return image{.width = x, .height = y, .channels_in_file = channels_in_file, .data = img_data};
}