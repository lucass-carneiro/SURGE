#include "logging.hpp"
#include "renderer.hpp"

#include <gsl/gsl-lite.hpp>

auto surge::renderer::smo::create(GLuint shader_program, const index_vertex_data &vd) noexcept
    -> indexed_context {
  log_info("Creating smo buffers");

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, gsl::narrow_cast<GLsizeiptr>(vd.vertex_data_size * sizeof(float)),
               vd.vertex_data, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               gsl::narrow_cast<GLsizeiptr>(vd.index_data_size * sizeof(GLuint)), vd.index_data,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return indexed_context{shader_program, VAO, VBO, EBO,
                         gsl::narrow_cast<GLsizei>(vd.index_data_size)};
}

auto surge::renderer::smo::create(GLuint shader_program, const vertex_data &vd) noexcept
    -> context {
  log_info("Creating smo buffers");

  GLuint VAO{0};
  GLuint VBO{0};

  glGenBuffers(1, &VBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, gsl::narrow_cast<GLsizeiptr>(vd.size * sizeof(float)), vd.data,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return context{shader_program, VAO, VBO, gsl::narrow_cast<GLsizei>(vd.size)};
}

auto surge::renderer::smo::create(const index_vertex_data &vd) noexcept
    -> tl::expected<indexed_context, error> {
  log_info("Creating smo shader");
  const auto shader{create_shader_program("shaders/smo.vert", "shaders/smo.frag")};
  if (!shader) {
    log_error("Unable to create smo shader");
    return tl::unexpected(error::shader_creation);
  } else {
    return create(*shader, vd);
  }
}

auto surge::renderer::smo::create(const vertex_data &vd) noexcept -> tl::expected<context, error> {
  log_info("Creating smo shader");
  const auto shader{create_shader_program("shaders/smo.vert", "shaders/smo.frag")};
  if (!shader) {
    log_error("Unable to create smo shader");
    return tl::unexpected(error::shader_creation);
  } else {
    return create(*shader, vd);
  }
}

void surge::renderer::smo::draw(const indexed_context &ctx, const draw_context &dctx) {
  glUseProgram(ctx.shader_program);

  uniforms::set(ctx.shader_program, "projection", dctx.projection);
  uniforms::set(ctx.shader_program, "view", dctx.view);
  uniforms::set(ctx.shader_program, "model", dctx.model);
  uniforms::set(ctx.shader_program, "color", dctx.color);

  glBindVertexArray(ctx.VAO);
  glDrawElements(GL_TRIANGLES, ctx.num_elements, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::renderer::smo::draw(type primitive_type, const context &ctx, const draw_context &dctx) {
  glUseProgram(ctx.shader_program);

  uniforms::set(ctx.shader_program, "projection", dctx.projection);
  uniforms::set(ctx.shader_program, "view", dctx.view);
  uniforms::set(ctx.shader_program, "model", dctx.model);
  uniforms::set(ctx.shader_program, "color", dctx.color);

  glBindVertexArray(ctx.VAO);
  glDrawArrays(static_cast<GLenum>(primitive_type), 0, ctx.num_elements);

  glBindVertexArray(0);
}