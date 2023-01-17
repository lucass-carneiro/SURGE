#ifndef SURGE_LOAD_TEXTURE_HPP
#define SURGE_LOAD_TEXTURE_HPP

#include "headers.hpp"
#include "image_loader.hpp"

#include <optional>

namespace surge {

template <surge_allocator alloc_t>
[[nodiscard]] inline auto load_texture(alloc_t *allocator, const std::filesystem::path &p,
                                       const char *ext) -> std::optional<GLuint> {

  // When passing images to OpenGL they must be flipped.
  stbi_set_flip_vertically_on_load(static_cast<int>(true));

  auto image{load_image(allocator, p, ext)};
  if (!image) {
    stbi_set_flip_vertically_on_load(static_cast<int>(false));
    glog<log_event::error>("Unable to load texture {}", p.c_str());
    return {};
  }

  stbi_set_flip_vertically_on_load(static_cast<int>(false));

  GLuint texture{0};
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  // Warpping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Filtering. TODO: Set via configs
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  const int format{image->channels_in_file == 4 ? GL_RGBA : GL_RGB};

  glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format, GL_UNSIGNED_BYTE,
               image->data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(allocator, image->data);

  return texture;
}

} // namespace surge

#endif // SURGE_LOAD_TEXTURE_HPP