#include "image_loader.hpp"
#include "log.hpp"
#include "options.hpp"

#include <filesystem>

/*#ifdef SURGE_STBIMAGE_ERRORS
#  define STBI_FAILURE_USERMSG
#else
#  define STBI_NO_FAILURE_STRINGS
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STBI_MALLOC(sz) surge::global_image_loader::get().get_palloc().malloc(sz)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STBI_REALLOC(p, newsz) surge::global_image_loader::get().get_palloc().realloc(p, newsz)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STBI_FREE(p) surge::global_image_loader::get().get_palloc().free(p)

#define STB_IMAGE_IMPLEMENTATION*/

#include "stb/stb_image.hpp"

void surge::global_image_loader::load_persistent(const std::filesystem::path &path) {

  int width{0}, height{0}, channels_in_file{0};
  stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels_in_file, 0);

  if (data == nullptr) {
    log_all<log_event::error>("stb_image error while loading a file: {}.", stbi_failure_reason());
    return;
  }

  stbi_image_free(data);
}