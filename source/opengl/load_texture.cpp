#include "opengl/load_texture.hpp"

#include "image_loader.hpp"
#include "logging_system/logging_system.hpp"

auto surge::load_texture(const std::filesystem::path &p, const char *ext) noexcept
    -> std::optional<GLuint> {

  // When passing images to OpenGL they must be flipped.
  stbi_set_flip_vertically_on_load(static_cast<int>(true));

  auto image{load_image(p, ext)};
  if (!image) {
    stbi_set_flip_vertically_on_load(static_cast<int>(false));
#ifdef SURGE_SYSTEM_Windows
    log_error(L"Unable to load texture {}", p.c_str());
#else
    log_error("Unable to load texture {}", p.c_str());
#endif
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

  stbi_image_free(image->data);

  return texture;
}