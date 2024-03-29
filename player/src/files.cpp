#include "files.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "options.hpp"

#include <fcntl.h>

#ifdef SURGE_SYSTEM_Windows
#  include <array>
#  include <gsl/gsl-lite.hpp>
#  include <io.h>
#endif

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz)           surge::allocators::mimalloc::malloc(sz)
#define STBI_REALLOC(p,newsz)     surge::allocators::mimalloc::realloc(p,newsz)
#define STBI_FREE(p)              surge::allocators::mimalloc::free(p)
#include <stb_image.h>
// clang-format on

#include <cstring>
#include <filesystem>
#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto surge::files::validate_path(const char *path) noexcept -> bool {
  try {
    const std::filesystem::path fs_opath{path};

    if (!std::filesystem::exists(fs_opath)) {
      log_error("The file %s does not exist.", path);
      return false;
    }

    if (!std::filesystem::is_regular_file(fs_opath)) {
      log_error("The path %s does not point to a regular file.", path);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    log_error("Error while validating file %s: %s", path, e.what());
    return false;
  }
}

#ifdef SURGE_SYSTEM_Windows

auto os_open_read(const char *path, void *buffer, unsigned int file_size) noexcept -> bool {
  std::array<char, 256> error_msg_buff{};
  error_msg_buff.fill('\0');

  int fd = 0;
  if (_sopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD) != 0) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Error while oppening file %s: %s", path, error_msg_buff.data());
    return false;
  }

  if (_read(fd, buffer, file_size) == -1) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Uanable to read the file %s: %s", path, error_msg_buff.data());
    _close(fd);
    return false;
  }

  _close(fd);

  return true;
}

#else

auto os_open_read(const char *path, void *buffer, unsigned int file_size) noexcept -> bool {
  // NOLINTNEXTLINE
  int fd = open(path, O_RDONLY);

  if (fd == -1) {
    log_error("Error while oppening file: %s", std::strerror(errno));
    return false;
  }

  if (read(fd, buffer, file_size) == -1) {
    log_error("Uanable to read the file %s: %s", path, std::strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return true;
}

#endif

auto surge::files::load_file(const char *path, bool append_null_byte) noexcept -> file {
  try {
    log_info("Loading raw data for file %s. Appending null byte: %s", path,
             append_null_byte ? "true" : "false");

    if (!validate_path(path)) {
      return tl::unexpected(error::invalid_path);
    }

    auto file_size{static_cast<unsigned int>(std::filesystem::file_size(path))};
    if (append_null_byte) {
      file_size += 1;
    }

    file_data_t buffer(file_size);
    buffer.reserve(file_size);
    std::fill(buffer.begin(), buffer.end(), std::byte{0});

    if (os_open_read(path, buffer.data(), file_size)) {
      return buffer;
    } else {
      return tl::unexpected(error::read_error);
    }

  } catch (const std::exception &e) {
    log_error("Unable to load file %s: %s", path, e.what());
    return tl::unexpected(error::unknow_error);
  }
}

auto surge::files::load_image(const char *p, bool flip) noexcept -> image {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::static_image::load_image");
#endif

  log_info("Loading image file %s", p);

  auto file{load_file(p, false)};
  if (!file) {
    log_error("Unable to load image file %s", p);
    return tl::unexpected(surge::error::static_image_load_error);
  }

  int iw{0}, ih{0}, channels_in_file{0};

  if (flip) {
    stbi_set_flip_vertically_on_load(static_cast<int>(true));
  }

  auto pixels{stbi_load_from_memory(static_cast<stbi_uc *>(static_cast<void *>(file->data())),
                                    gsl::narrow_cast<int>(file.value().size()), &iw, &ih,
                                    &channels_in_file, 0)};

  if (flip) {
    stbi_set_flip_vertically_on_load(static_cast<int>(false));
  }

  if (pixels == nullptr) {
    log_error("Unable to load image file %s due to stbi error: %s", p, stbi_failure_reason());
    return tl::unexpected(surge::error::static_image_stbi_error);
  }

  return image_data{iw, ih, channels_in_file, pixels, p};
}

void surge::files::free_image(image_data &image) noexcept { stbi_image_free(image.pixels); }

auto surge::files::load_image_task(const char *path, bool flip) noexcept -> img_future {
  // This option has to be set before the parallel tasks because STBI implements it as a global
  // variable
  stbi_set_flip_vertically_on_load(static_cast<int>(flip));
  return tasks::executor().async([=]() { return load_image(path, false); });
}

void surge::files::free_image_task(image_data &image) noexcept {
  tasks::executor().silent_async([=]() { stbi_image_free(image.pixels); });
}
