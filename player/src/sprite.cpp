#include "sprite.hpp"

#include "logging.hpp"
#include "mpsb.hpp"

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
  glNamedBufferStorage(MMB, static_cast<GLsizeiptr>(sizeof(glm::mat4) * max_sprites), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite texture alphas buffer");
  GLuint AVB{0};
  glCreateBuffers(1, &AVB);
  glNamedBufferStorage(AVB, static_cast<GLsizeiptr>(sizeof(float) * max_sprites), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite texture handles buffer");
  GLuint THB{0};
  glCreateBuffers(1, &THB);
  glNamedBufferStorage(THB, static_cast<GLsizeiptr>(sizeof(GLuint64) * max_sprites), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

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
  const GLenum internal_format{image.channels == 4 ? GLenum{GL_RGBA8} : GLenum{GL_RGB8}};
  const GLenum format{image.channels == 4 ? GLenum{GL_RGBA} : GLenum{GL_RGB}};

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
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::destroy_texture");
  TracyGpuZone("GPU surge::atom::sprite::destroy_texture");
#endif

  glDeleteTextures(1, &texture);
}

void surge::atom::sprite::destroy_texture(const vector<GLuint> &texture) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::destroy_texture(vector)");
  TracyGpuZone("GPU surge::atom::sprite::destroy_texture(vector)");
#endif

  glDeleteTextures(static_cast<GLsizei>(texture.size()), texture.data());
}

void surge::atom::sprite::make_resident(GLuint64 handles) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::make_resident");
  TracyGpuZone("GPU surge::atom::sprite::make_resident");
#endif

  glMakeTextureHandleResidentARB(handles);
}

void surge::atom::sprite::make_non_resident(GLuint64 handles) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::make_non_resident");
  TracyGpuZone("GPU surge::atom::sprite::make_non_resident");
#endif

  glMakeTextureHandleNonResidentARB(handles);
}

void surge::atom::sprite::make_resident(const vector<GLuint64> &texture_handles) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::make_resident(vector)");
  TracyGpuZone("GPU surge::atom::sprite::make_resident(vector)");
#endif

  for (auto handle : texture_handles) {
    glMakeTextureHandleResidentARB(handle);
  }
}

void surge::atom::sprite::make_non_resident(const vector<GLuint64> &texture_handles) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::make_non_resident(vector)");
  TracyGpuZone("GPU surge::atom::sprite::make_non_resident(vector)");
#endif

  for (auto handle : texture_handles) {
    glMakeTextureHandleNonResidentARB(handle);
  }
}

auto surge::atom::sprite::is_resident(GLuint64 handle) noexcept -> bool {
  return glIsTextureHandleResidentARB(handle);
}

void surge::atom::sprite::send_buffers(const buffer_data &bd, const data_list &dl) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::send_buffers");
  TracyGpuZone("GPU surge::atom::sprite::send_buffers");
#endif

  if (dl.texture_handles.size() != 0 && dl.models.size() != 0 && dl.alphas.size() != 0) {
    glNamedBufferSubData(bd.MMB, 0, static_cast<GLsizeiptr>(sizeof(glm::mat4) * dl.models.size()),
                         dl.models.data());

    glNamedBufferSubData(bd.AVB, 0, static_cast<GLsizeiptr>(sizeof(float) * dl.alphas.size()),
                         dl.alphas.data());

    glNamedBufferSubData(bd.THB, 0,
                         static_cast<GLsizeiptr>(sizeof(GLuint64) * dl.texture_handles.size()),
                         dl.texture_handles.data());
  }
}

void surge::atom::sprite::draw(const GLuint &sp, const buffer_data &bd, const GLuint &MPSB,
                               const data_list &dl) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::draw");
  TracyGpuZone("GPU surge::atom::sprite::draw");
#endif

  if (dl.texture_handles.size() != 0 && dl.models.size() != 0 && dl.alphas.size() != 0) {

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, MPSB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bd.MMB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bd.AVB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bd.THB);

    glUseProgram(sp);

    glBindVertexArray(bd.VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(dl.models.size()));
  }
}