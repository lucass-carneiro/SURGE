#include "sprite.hpp"

#include "logging.hpp"

#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::sprite::create_buffers() noexcept -> buffer_data {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::create_buffers");
  TracyGpuZone("GPU surge::atom::sprite::create_buffers");
#endif

  /***************
   * Gen Buffers *
   ***************/
  log_info("Creating sprite buffers");

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  /***************
   * Create quad *
   ***************/
  log_info("Creating sprite base quad");

  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  return buffer_data{VBO, EBO, VAO};
}

void surge::atom::sprite::destroy_buffers(const buffer_data &bd) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::destroy_buffers");
  TracyGpuZone("GPU surge::atom::sprite::destroy_buffers");
#endif

  log_info("Deleting sprite buffer data (%u, %u, %u)", bd.VBO, bd.EBO, bd.VAO);

  glDeleteBuffers(1, &(bd.VBO));
  glDeleteBuffers(1, &(bd.EBO));
  glDeleteVertexArrays(1, &(bd.VAO));
}

auto surge::atom::sprite::create_texture(const files::image &image,
                                         renderer::texture_filtering filtering,
                                         renderer::texture_wrap wrap) noexcept
    -> tl::expected<GLuint64, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::create_texture");
  TracyGpuZone("GPU surge::atom::sprite::create_texture");
#endif
  log_info("Creating sprite texture");

  GLuint texture{0};
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);

  // Warpping
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, gsl::narrow_cast<GLint>(wrap));
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, gsl::narrow_cast<GLint>(wrap));

  // Filtering
  glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, gsl::narrow_cast<GLint>(filtering));
  glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, gsl::narrow_cast<GLint>(filtering));

  // Loading and mip mapping
  const int internal_format{image.channels == 4 ? GL_RGBA8 : GL_RGB8};
  const int format{image.channels == 4 ? GL_RGBA : GL_RGB};

  glTextureStorage2D(texture, 1, internal_format, image.width, image.height);
  glTextureSubImage2D(texture, 0, 0, 0, image.width, image.height, format, GL_UNSIGNED_BYTE,
                      image.texels);
  glGenerateTextureMipmap(texture);

  const auto handle{glGetTextureHandleARB(texture)};
  if (handle == 0) {
    log_error("Unable to create texture handle");
    return error::texture_handle_creation;
  }

  return handle;
}

void surge::atom::sprite::make_resident(GLuint64 handles) noexcept {
  glMakeTextureHandleResidentARB(handles);
}

void surge::atom::sprite::make_non_resident(GLuint64 handles) noexcept {
  glMakeTextureHandleNonResidentARB(handles);
}

void surge::atom::sprite::make_resident(const vector<GLuint64> &texture_handles) noexcept {
  for (auto handle : texture_handles) {
    glMakeTextureHandleResidentARB(handle);
  }
}

void surge::atom::sprite::make_non_resident(const vector<GLuint64> &texture_handles) noexcept {
  for (auto handle : texture_handles) {
    glMakeTextureHandleNonResidentARB(handle);
  }
}

void surge::atom::sprite::draw(const GLuint &sp, const buffer_data &bd, const glm::mat4 &proj,
                               const glm::mat4 &view, const vector<glm::mat4> &models,
                               const vector<GLuint64> &texture_handles,
                               const vector<float> &alphas) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::draw");
  TracyGpuZone("GPU surge::atom::sprite::draw");
#endif
  if (models.size() == 0) {
    return;
  }

  glUseProgram(sp);

  renderer::uniforms::set(sp, "projection", proj);
  renderer::uniforms::set(sp, "view", view);

  renderer::uniforms::set(sp, "models", models.data(), models.size());
  renderer::uniforms::set(sp, "textures", texture_handles.data(), texture_handles.size());
  renderer::uniforms::set(sp, "alphas", alphas.data(), alphas.size());

  glBindVertexArray(bd.VAO);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                          gsl::narrow_cast<GLsizei>(models.size()));
}