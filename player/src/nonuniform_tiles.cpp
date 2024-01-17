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

  glUseProgram(shader_program);

  renderer::uniforms::set(shader_program, "projection", dctx.projection);
  renderer::uniforms::set(shader_program, "view", dctx.view);

  renderer::uniforms::set(shader_program, "positions", dctx.positions.data(),
                          dctx.positions.size());
  renderer::uniforms::set(shader_program, "scales", dctx.scales.data(), dctx.scales.size());

  glBindVertexArray(ctx.VAO);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, dctx.positions.size());
}

void surge::atom::nonuniform_tiles::cleanup(buffer_data &ctx) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::static_image::cleanup");
  TracyGpuZone("GPU surge::atom::static_image::cleanup");
#endif
  log_info("Deleting nonuniform tile buffer data (%u, %u, %u)", ctx.VBO, ctx.EBO, ctx.VAO);

  glDeleteBuffers(1, &(ctx.VBO));
  glDeleteBuffers(1, &(ctx.EBO));
  glDeleteVertexArrays(1, &(ctx.VAO));
}