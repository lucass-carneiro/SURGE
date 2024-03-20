#include "texture.hpp"

#include <gsl/gsl-lite.hpp>

surge::atom::texture::record::record(usize initial_size) {
  names.reserve(initial_size);
  ids.reserve(initial_size);
  handles.reserve(initial_size);
}

auto surge::atom::texture::record::img_to_texture(const load_options &opts,
                                                  const files::image_data &image) noexcept
    -> texture_result_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::texture::record::img_to_texture");
  TracyGpuZone("GPU surge::atom::texture::record::img_to_texture");
#endif

  log_info("Creating OpenGL texture");

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  GLuint texture{0};
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);

  // Warpping
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, gsl::narrow_cast<GLint>(opts.wrap));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, gsl::narrow_cast<GLint>(opts.wrap));

  // Filtering GL_LINEAR_MIPMAP_LINEAR

  switch (opts.filtering) {
  case renderer::texture_filtering::nearest:
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    break;

  case renderer::texture_filtering::linear:
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    break;

  default:
    break;
  }

  glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, gsl::narrow_cast<GLint>(opts.filtering));

  if (opts.filtering == renderer::texture_filtering::anisotropic) {
    GLfloat max_aniso{0};
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
    glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
  }

  // Loading and mip mapping
  const GLenum internal_format{image.channels == 4 ? GLenum{GL_RGBA8} : GLenum{GL_RGB8}};
  const GLenum format{image.channels == 4 ? GLenum{GL_RGBA} : GLenum{GL_RGB}};

  glTextureStorage2D(texture, opts.mipmap_levels, internal_format, image.width, image.height);
  glTextureSubImage2D(texture, 0, 0, 0, image.width, image.height, format, GL_UNSIGNED_BYTE,
                      image.pixels);

  glGenerateTextureMipmap(texture);

  const auto handle{glGetTextureHandleARB(texture)};
  if (handle == 0) {
    log_error("Unable to create texture handle");
    return tl::unexpected{error::texture_handle_creation};
  }

  return std::make_tuple(texture, handle);
}

void surge::atom::texture::record::reside_all_unchecked() noexcept {
  for (const auto &handle : handles) {
    glMakeTextureHandleResidentARB(handle);
  }
}

void surge::atom::texture::record::reside_all() noexcept {
  for (const auto &handle : handles) {
    if (!glIsTextureHandleResidentARB(handle)) {
      glMakeTextureHandleResidentARB(handle);
    }
  }
}

void surge::atom::texture::record::unreside_all() noexcept {
  for (const auto &handle : handles) {
    if (glIsTextureHandleResidentARB(handle)) {
      glMakeTextureHandleNonResidentARB(handle);
    }
  }
}

void surge::atom::texture::record::unload_all() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::texture::record::unload_all");
  TracyGpuZone("GPU surge::atom::texture::record::unload_all");
#endif

  unreside_all();

  // Delete all textures
  for (const auto &id : ids) {
    glDeleteTextures(1, &id);
  }

  // Reset record arrays.
  names.clear();
  ids.clear();
  handles.clear();
}

auto surge::atom::texture::record::find(const char *file_name) noexcept -> std::optional<GLuint64> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::texture::record::find");
#endif
  const auto pos_it{std::find(names.begin(), names.end(), file_name)};
  if (pos_it == std::end(names)) {
    return {};
  } else {
    const auto idx{static_cast<usize>(pos_it - names.begin())};
    return handles[idx];
  }
}