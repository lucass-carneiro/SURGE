#include "sprite.hpp"

#include "logging.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::sprite::database::create(usize max_sprites) noexcept -> database {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::create");
  TracyGpuZone("GPU surge::atom::sprite::database::create");
#endif
  log_info("Creating sprite database");

  database db;

  /***************
   * Gen Buffers *
   ***************/

  glCreateVertexArrays(1, &db.VAO);
  glCreateBuffers(1, &db.VBO);
  glCreateBuffers(1, &db.EBO);

  /***************
   * Create quad *
   ***************/
  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(db.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, db.VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, db.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  /***************
   * Create GBAs *
   ***************/
  db.texture_handles = gba<GLuint64>::create(max_sprites, "Sprite Texture Handles GBA");
  db.models = gba<glm::mat4>::create(max_sprites, "Sprite Modesl GBA");
  db.alphas = gba<float>::create(max_sprites, "Sprite Alphas GBA");

  return db;
}

void surge::atom::sprite::database::destroy() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::destroy");
  TracyGpuZone("GPU surge::atom::sprite::database::destroy");
#endif
  log_info("Destroying sprite database");

  alphas.destroy();
  models.destroy();
  texture_handles.destroy();
  glDeleteBuffers(1, &(EBO));
  glDeleteBuffers(1, &(VBO));
  glDeleteVertexArrays(1, &(VAO));
}

void surge::atom::sprite::database::add(GLuint64 handle, glm::mat4 model, float alpha) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::add");
#endif
  texture_handles.push(handle);
  models.push(model);
  alphas.push(alpha);
}

void surge::atom::sprite::database::reset() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::reset()");
#endif
  texture_handles.reset();
  models.reset();
  alphas.reset();
}

void surge::atom::sprite::database::draw(const GLuint &sp) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::sprite::database::draw");
  TracyGpuZone("GPU surge::atom::sprite::database::draw");
#endif

  if (texture_handles.size() != 0 && models.size() != 0 && alphas.size() != 0) {

    models.bind(GL_SHADER_STORAGE_BUFFER, 3);
    alphas.bind(GL_SHADER_STORAGE_BUFFER, 4);
    texture_handles.bind(GL_SHADER_STORAGE_BUFFER, 5);

    glUseProgram(sp);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(models.size()));

    texture_handles.lock();
    alphas.lock();
    models.lock();
  }
}

auto surge::atom::sprite::place(glm::vec2 &&pos, glm::vec2 &&scale, float z) noexcept -> glm::mat4 {
  const auto mv{glm::vec3{std::move(pos), z}};
  const auto sc{glm::vec3{std::move(scale), 1.0f}};
  return glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc);
}

auto surge::atom::sprite::place(const glm::vec2 &pos, const glm::vec2 &scale, float z) noexcept
    -> glm::mat4 {
  const auto mv{glm::vec3{pos, z}};
  const auto sc{glm::vec3{scale, 1.0f}};
  return glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc);
}