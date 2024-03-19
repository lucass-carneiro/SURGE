#include "sprite.hpp"

#include "logging.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

surge::atom::sprite::record::record(usize max_sprts) : max_sprites{max_sprts} {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::record::record");
#endif

  texture_handles.reserve(max_sprites);
  models.reserve(max_sprites);
  alphas.reserve(max_sprites);
}

void surge::atom::sprite::record::reset() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::record::reset()");
#endif

  texture_handles.clear();
  models.clear();
  alphas.clear();
}

void surge::atom::sprite::record::create_buffers() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::record::create_buffers");
  TracyGpuZone("GPU surge::atom::sprite::record::create_buffers");
#endif

  /***************
   * Gen Buffers *
   ***************/
  log_info("Creating sprite record buffers");

  glCreateBuffers(1, &VBO);
  glCreateBuffers(1, &EBO);
  glCreateVertexArrays(1, &VAO);

  /***************
   * Create quad *
   ***************/
  log_info("Creating sprite record base quad");

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
  log_info("Creating sprite record model matrices buffer");
  glCreateBuffers(1, &MMB);
  glNamedBufferStorage(MMB, static_cast<GLsizeiptr>(sizeof(glm::mat4) * max_sprites), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite record texture alphas buffer");
  glCreateBuffers(1, &AVB);
  glNamedBufferStorage(AVB, static_cast<GLsizeiptr>(sizeof(float) * max_sprites), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite record texture handles buffer");
  glCreateBuffers(1, &THB);
  glNamedBufferStorage(THB, static_cast<GLsizeiptr>(sizeof(GLuint64) * max_sprites), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);
}

void surge::atom::sprite::record::destroy_buffers() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::record::destroy_buffers");
  TracyGpuZone("GPU surge::atom::sprite::record::destroy_buffers");
#endif

  log_info("Deleting sprite record buffer data\n  VBO: %u\n  EBO: %u\n  VAO: %u\n  MMB: %u\n  AVB: "
           "%u\n  "
           "THB: %u",
           VBO, EBO, VAO, MMB, AVB, THB);

  glDeleteBuffers(1, &(VBO));
  glDeleteBuffers(1, &(EBO));
  glDeleteVertexArrays(1, &(VAO));

  glDeleteBuffers(1, &(MMB));
  glDeleteBuffers(1, &(AVB));
  glDeleteBuffers(1, &(THB));
}

void surge::atom::sprite::record::sync_buffers() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::record::sync_buffers");
  TracyGpuZone("GPU surge::atom::sprite::record::sync_buffers");
#endif

  if (texture_handles.size() != 0 && models.size() != 0 && alphas.size() != 0) {
    glNamedBufferSubData(MMB, 0, static_cast<GLsizeiptr>(sizeof(glm::mat4) * models.size()),
                         models.data());

    glNamedBufferSubData(AVB, 0, static_cast<GLsizeiptr>(sizeof(float) * alphas.size()),
                         alphas.data());

    glNamedBufferSubData(THB, 0, static_cast<GLsizeiptr>(sizeof(GLuint64) * texture_handles.size()),
                         texture_handles.data());
  }
}

void surge::atom::sprite::record::add(glm::mat4 &&model_matrix, GLuint64 &&texture_handle,
                                      float &&alpha) noexcept {
  models.push_back(std::move(model_matrix));
  texture_handles.push_back(std::move(texture_handle));
  alphas.push_back(std::move(alpha));
}

void surge::atom::sprite::record::draw(const GLuint &sp, const GLuint &MPSB) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::record::draw");
  TracyGpuZone("GPU surge::atom::sprite::record::draw");
#endif

  if (texture_handles.size() != 0 && models.size() != 0 && alphas.size() != 0) {

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, MPSB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, MMB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, AVB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, THB);

    glUseProgram(sp);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(models.size()));
  }
}