#include "sc_files.hpp"

#include "sc_allocators.hpp"
#include "sc_logging.hpp"
#include "sc_options.hpp"

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

#include <OpenEXR/ImfRgbaFile.h>
#include <Imath/ImathVec.h>
#include <Imath/ImathBox.h>
// clang-format on

#include <cstring>
#include <filesystem>
#include <gsl/gsl-lite.hpp>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto surge::files::validate_path(const char *path) -> bool {
  try {
    const std::filesystem::path fs_opath{path};

    if (!std::filesystem::exists(fs_opath)) {
      log_error("The file {} does not exist.", path);
      return false;
    }

    if (!std::filesystem::is_regular_file(fs_opath)) {
      log_error("The path {} does not point to a regular file.", path);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    log_error("Error while validating file {}: {}", path, e.what());
    return false;
  }
}

#ifdef SURGE_SYSTEM_Windows

auto os_open_read(const char *path, void *buffer, unsigned int file_size) -> bool {
  std::array<char, 256> error_msg_buff{};
  error_msg_buff.fill('\0');

  int fd = 0;
  if (_sopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD) != 0) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Error while oppening file {}: {}", path, error_msg_buff.data());
    return false;
  }

  if (_read(fd, buffer, file_size) == -1) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Uanable to read the file {}: {}", path, error_msg_buff.data());
    _close(fd);
    return false;
  }

  _close(fd);

  return true;
}

#else

auto os_open_read(const char *path, void *buffer, unsigned int file_size) -> bool {
  // NOLINTNEXTLINE
  int fd = open(path, O_RDONLY);

  if (fd == -1) {
    log_error("Error while oppening file: {}", std::strerror(errno));
    return false;
  }

  if (read(fd, buffer, file_size) == -1) {
    log_error("Uanable to read the file {}: {}", path, std::strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return true;
}

#endif

auto surge::files::load_file(const char *path, bool append_null_byte) -> file {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::files::load_file");
#endif

  try {
    log_info("Loading raw data for file {}. Appending null byte: {}", path,
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
    log_error("Unable to load file {}: {}", path, e.what());
    return tl::unexpected(error::unknow_error);
  }
}

auto surge::files::load_image(const char *p, bool flip) -> image {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::files::load_image");
#endif

  log_info("Loading image file {}", p);

  auto file{load_file(p, false)};
  if (!file) {
    log_error("Unable to load image file {}", p);
    return tl::unexpected(surge::error::image_load_error);
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
    log_error("Unable to load image file {} due to stbi error: {}", p, stbi_failure_reason());
    return tl::unexpected(surge::error::image_stbi_error);
  }

  return image_data{iw, ih, channels_in_file, pixels, p};
}

auto surge::files::load_openEXR(const char *p) -> openEXR_image {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::files::load_openEXR");
#endif

  log_info("Loading OpenEXR image file {}", p);

  // TODO: Try to use a lower level API for reading these files.
  // TODO: This image needs to be fliped around the y axis. Figure out how to do that. For now, flip
  // it in blender See:
  //
  // https: //
  // developer.nvidia.com/gpugems/gpugems/part-iv-image-processing/chapter-26-openexr-image-file-format
  try {
    Imf::RgbaInputFile in(p);

    Imath::Box2i win = in.dataWindow();
    Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);

    auto pixel_buffer{allocators::mimalloc::malloc(sizeof(Imf::Rgba) * static_cast<usize>(dim.x)
                                                   * static_cast<usize>(dim.y))};

    int dx = win.min.x;
    int dy = win.min.y;

    // NOLINTNEXTLINE
    in.setFrameBuffer(static_cast<Imf::Rgba *>(pixel_buffer) - dx - dy * dim.x, 1,
                      static_cast<usize>(dim.x));
    in.readPixels(win.min.y, win.max.y);

    return openEXR_image_data{dim.x, dim.y, pixel_buffer, p};

  } catch (std::exception &e) {
    log_error("Unable to load {}: {}", p, e.what());
    return tl::unexpected{error::openEXR_exception};
  }
}

void surge::files::free_openEXR(openEXR_image_data &data) {
  allocators::mimalloc::free(data.pixels);
}

void surge::files::free_image(image_data &image) { stbi_image_free(image.pixels); }

auto surge::files::load_image_task(const char *path, bool flip) -> img_future {
  // This option has to be set before the parallel tasks because STBI implements it as a global
  // variable
  stbi_set_flip_vertically_on_load(static_cast<int>(flip));
  return tasks::executor::get().async([=]() { return load_image(path, false); });
}

void surge::files::free_image_task(image_data &image) {
  tasks::executor::get().silent_async([=]() { stbi_image_free(image.pixels); });
}