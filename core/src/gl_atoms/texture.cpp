#include "gl_atoms/texture.hpp"

#include <cstring>
#include <gsl/gsl-lite.hpp>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

static constexpr XXH64_hash_t hash_seed{100};

auto surge::gl_atom::texture::from_image(const create_info &ci,
                                         const files::image_data &img) noexcept -> create_t {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::texture::create_from_image");
  TracyGpuZone("GPU surge::gl_atom::texture::create_from_image");
#endif

  using std::strlen;

  log_info("Creating OpenGL texture from image {}", img.file_name);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  GLuint texture{0};
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);

  // Warpping
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, gsl::narrow_cast<GLint>(ci.wrap));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, gsl::narrow_cast<GLint>(ci.wrap));

  // Filtering
  switch (ci.filtering) {
  case texture_filtering::nearest:
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    break;

  case texture_filtering::linear:
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    break;

  case texture_filtering::anisotropic: {
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat max_aniso{0};
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
    glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
    break;
  }

  default:
    break;
  }

  // Loading and mip mapping
  const GLenum internal_format{img.channels == 4 ? GLenum{GL_RGBA8} : GLenum{GL_RGB8}};
  const GLenum format{img.channels == 4 ? GLenum{GL_RGBA} : GLenum{GL_RGB}};
  const auto type{GL_UNSIGNED_BYTE};

  glTextureStorage2D(texture, ci.mipmap_levels, internal_format, img.width, img.height);
  glTextureSubImage2D(texture, 0, 0, 0, img.width, img.height, format, type, img.pixels);

  glGenerateTextureMipmap(texture);

  const auto handle{glGetTextureHandleARB(texture)};
  if (handle == 0) {
    log_error("Unable to create texture handle");
    return tl::unexpected{error::texture_handle_creation};
  }

  if (ci.make_resident) {
    make_resident(handle);
  }

  return create_data{texture, handle, XXH64(img.file_name, strlen(img.file_name), hash_seed)};
}

auto surge::gl_atom::texture::from_openEXR(const create_info &ci,
                                           const files::openEXR_image_data &img) noexcept
    -> create_t {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::texture::from_openEXR");
  TracyGpuZone("GPU surge::gl_atom::texture::from_openEXR");
#endif

  using std::strlen;

  log_info("Creating OpenGL texture from OpenEXR image {}", img.file_name);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  GLuint texture{0};
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);

  // Warpping
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, gsl::narrow_cast<GLint>(ci.wrap));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, gsl::narrow_cast<GLint>(ci.wrap));

  // Filtering
  switch (ci.filtering) {
  case texture_filtering::nearest:
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    break;

  case texture_filtering::linear:
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    break;

  case texture_filtering::anisotropic: {
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat max_aniso{0};
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
    glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);
    break;
  }

  default:
    break;
  }

  // Loading and mip mapping
  const auto internal_format{GL_RGBA16F};
  const auto format{GL_RGBA};
  const auto type{GL_HALF_FLOAT};

  glTextureStorage2D(texture, ci.mipmap_levels, internal_format, img.width, img.height);
  glTextureSubImage2D(texture, 0, 0, 0, img.width, img.height, format, type, img.pixels);

  glGenerateTextureMipmap(texture);

  const auto handle{glGetTextureHandleARB(texture)};
  if (handle == 0) {
    log_error("Unable to create texture handle");
    log_error("{} {} {}", img.width, img.height, ci.mipmap_levels);
    return tl::unexpected{error::texture_handle_creation};
  }

  if (ci.make_resident) {
    make_resident(handle);
  }

  return create_data{texture, handle, XXH64(img.file_name, strlen(img.file_name), hash_seed)};
}

void surge::gl_atom::texture::destroy(create_data &cd) noexcept {
  destroy(cd.id, cd.handle);
  cd.id = 0;
  cd.handle = 0;
  cd.name_hash = 0;
}

void surge::gl_atom::texture::destroy(GLuint id, GLuint64 handle) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::texture::destroy");
  TracyGpuZone("GPU surge::gl_atom::texture::destroy");
#endif
  make_non_resident(handle);
  glDeleteTextures(1, &id);
}

void surge::gl_atom::texture::make_resident(GLuint64 handle) noexcept {
  if (handle != 0 && !glIsTextureHandleResidentARB(handle)) {
    glMakeTextureHandleResidentARB(handle);
  }
}

void surge::gl_atom::texture::make_non_resident(GLuint64 handle) noexcept {
  if (handle != 0 && glIsTextureHandleResidentARB(handle)) {
    glMakeTextureHandleNonResidentARB(handle);
  }
}

auto surge::gl_atom::texture::database::create(usize initial_size) noexcept -> database {
  log_info("Creating texture database");

  database db;
  db.ids.reserve(initial_size);
  db.handles.reserve(initial_size);
  db.name_hashes.reserve(initial_size);

  return db;
}

void surge::gl_atom::texture::database::destroy() noexcept {
  log_info("Destroying texture database");
  reset();
}

auto surge::gl_atom::texture::database::find(const char *file_name) const noexcept
    -> std::optional<GLuint64> {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::texture::database::find");
#endif
  using std::strlen;

  const auto query_hash{XXH64(file_name, strlen(file_name), hash_seed)};
  const auto pos_it{std::find(name_hashes.begin(), name_hashes.end(), query_hash)};

  if (pos_it == std::end(name_hashes)) {
    return {};
  } else {
    const auto idx{static_cast<usize>(pos_it - name_hashes.begin())};
    return handles[idx];
  }
}

void surge::gl_atom::texture::database::reset() noexcept {
  for (usize i = 0; i < handles.size(); i++) {
    texture::destroy(ids[i], handles[i]);
  }

  ids.clear();
  handles.clear();
  name_hashes.clear();
}

void surge::gl_atom::texture::database::add_openEXR(const create_info &ci,
                                                    const char *path) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::texture::database::add_openEXR");
  TracyGpuZone("GPU surge::gl_atom::texture::database::add_openEXR");
#endif

  auto img{files::load_openEXR(path)};

  if (img) {
    const auto texture_data{from_openEXR(ci, *img)};
    if (texture_data) {
      ids.push_back(texture_data->id);
      handles.push_back(texture_data->handle);
      name_hashes.push_back(texture_data->name_hash);
    } else {
      log_error("Unable to create texture from {}", img->file_name);
    }
    files::free_openEXR(*img);
  }
}