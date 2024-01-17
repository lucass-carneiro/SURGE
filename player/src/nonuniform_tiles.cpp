#include "nonuniform_tiles.hpp"

#include "logging.hpp"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::atom::nonuniform_tiles::create(const char *, renderer::texture_filtering) noexcept
    -> tl::expected<buffer_data, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::nonuniform_tiles::create");
  TracyGpuZone("GPU surge::atom::nonuniform_tiles::create");
#endif

  /***************
   * Gen Buffers *
   ***************/
  log_info("Creating nonuniform tiles buffers");

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  /***************
   * Create quad *
   ***************/
  log_info("Creating nonuniform tiles base quad");

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
                        reinterpret_cast<const void *>(3 * sizeof(float)));

  return buffer_data{VBO, EBO, VAO};
}

void surge::atom::nonuniform_tiles::draw(GLuint shader_program, const buffer_data &ctx,
                                         const draw_data &dctx) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::nonuniform_tiles::draw");
  TracyGpuZone("GPU surge::atom::nonuniform_tiles::draw");
#endif

  const auto model_0{glm::scale(glm::translate(glm::mat4{1.0f}, dctx.pos), dctx.scale)};
  const auto model_1{glm::scale(
      glm::translate(glm::mat4{1.0f}, (dctx.pos + glm::vec3{100.0f, 100.0f, 0.0f})), dctx.scale)};

  std::array<glm::mat4, 2> models{};
  models[0] = model_0;
  models[1] = model_1;

  glUseProgram(shader_program);

  renderer::uniforms::set(shader_program, "projection", dctx.projection);
  renderer::uniforms::set(shader_program, "view", dctx.view);

  glUniformMatrix4fv(glGetUniformLocation(shader_program, "models"), 2, GL_FALSE,
                     glm::value_ptr(models[0]));

  glBindVertexArray(ctx.VAO);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 2);
}