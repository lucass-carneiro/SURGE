#include "sprite.hpp"

#include "logging.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::sprite::create_buffers(usize max_sprites) noexcept -> buffer_data {
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

  /****************
   * Create SSBOs *
   ****************/
  log_info("Creating sprite model matrices buffer");
  GLuint MMB{0};
  glCreateBuffers(1, &MMB);
  glNamedBufferStorage(MMB, sizeof(glm::mat4) * max_sprites, nullptr, GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite model texture alphas buffer");
  GLuint AVB{0};
  glCreateBuffers(1, &AVB);
  glNamedBufferStorage(AVB, sizeof(float) * max_sprites, nullptr, GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite texture buffer");
  GLuint THB{0};
  glCreateBuffers(1, &THB);
  glNamedBufferStorage(THB, sizeof(GLuint64) * max_sprites, nullptr, GL_DYNAMIC_STORAGE_BIT);

  return buffer_data{VBO, EBO, VAO, MMB, AVB, THB};
}

void surge::atom::sprite::destroy_buffers(const buffer_data &bd) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::destroy_buffers");
  TracyGpuZone("GPU surge::atom::sprite::destroy_buffers");
#endif

  log_info("Deleting sprite buffer data\n  VBO: %u\n  EBO: %u\n  VAO: %u\n  MMB: %u\n  AVB: %u\n  "
           "THB: %u",
           bd.VBO, bd.EBO, bd.VAO, bd.MMB, bd.AVB, bd.THB);

  glDeleteBuffers(1, &(bd.VBO));
  glDeleteBuffers(1, &(bd.EBO));
  glDeleteVertexArrays(1, &(bd.VAO));

  glDeleteBuffers(1, &(bd.MMB));
  glDeleteBuffers(1, &(bd.AVB));
  glDeleteBuffers(1, &(bd.THB));
}

auto surge::atom::sprite::create_texture(const files::image &image,
                                         renderer::texture_filtering filtering,
                                         renderer::texture_wrap wrap) noexcept
    -> tl::expected<std::tuple<GLuint, GLuint64>, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::create_texture");
  TracyGpuZone("GPU surge::atom::sprite::create_texture");
#endif
  log_info("Creating sprite texture");

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

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
    return tl::unexpected{error::texture_handle_creation};
  }

  return std::make_tuple(texture, handle);
}

void surge::atom::sprite::destroy_texture(GLuint texture) noexcept {
  glDeleteTextures(1, &texture);
}

void surge::atom::sprite::destroy_texture(const vector<GLuint> &texture) noexcept {
  glDeleteTextures(texture.size(), texture.data());
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

void surge::atom::sprite::send_buffers(const buffer_data &bd, const data_list &dl) noexcept {
  glNamedBufferSubData(bd.MMB, 0, sizeof(glm::mat4) * dl.models.size(), dl.models.data());
  glNamedBufferSubData(bd.AVB, 0, sizeof(float) * dl.alphas.size(), dl.alphas.data());
  glNamedBufferSubData(bd.THB, 0, sizeof(GLuint64) * dl.texture_handles.size(),
                       dl.texture_handles.data());
}

void surge::atom::sprite::draw(const GLuint &sp, const buffer_data &bd, const glm::mat4 &proj,
                               const glm::mat4 &view, const data_list &dl) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::draw");
  TracyGpuZone("GPU surge::atom::sprite::draw");
#endif

  if (dl.models.size() == 0) {
    return;
  }

  glUseProgram(sp);

  renderer::uniforms::set(sp, "projection", proj);
  renderer::uniforms::set(sp, "view", view);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bd.MMB);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bd.AVB);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bd.THB);

  glBindVertexArray(bd.VAO);

  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                          gsl::narrow_cast<GLsizei>(dl.models.size()));
}