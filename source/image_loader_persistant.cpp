#include "allocators/global_allocators.hpp"
#include "file.hpp"
#include "image_loader.hpp"
#include "log.hpp"
#include "options.hpp"
#include "safe_ops.hpp"
#include "stb/stb_image.hpp"

#include <filesystem>

#ifdef SURGE_STBIMAGE_ERRORS
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

#define STB_IMAGE_IMPLEMENTATION

void surge::global_image_loader::load_persistent(const std::filesystem::path &) {
  /*const auto fs = std::filesystem::file_size(path);
  void *buffer = global_stack_allocator::get().malloc(fs);
  load_to_mem(path, ".png", fs, buffer);

  int x = 0, y = 0, channels_in_file = 0;
  const auto state = global_stack_allocator::get().save();
  auto img = stbi_load_from_memory(static_cast<const stbi_uc *>(buffer),
                                   safe_cast<int>(fs).value_or(0), &x, &y, &channels_in_file, 0);

  if (img == nullptr) {
    glog<log_event::error>("Stbi error: {}", stbi_failure_reason());
  }

  // clang-format off
  glog<log_event::message>("Read PNG image file {}:\n"
                              "  width: {}\n"
                              "  height: {}\n"
                              "  channels {}",
                              path.c_str(),
                              x,
                              y,
                              channels_in_file);
  // clang-format off

  stbi_image_free(img);
  global_stack_allocator::get().restore(state);
  global_stack_allocator::get().free(buffer);*/
}