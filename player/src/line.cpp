#include "logging.hpp"
#include "renderer.hpp"

// clang-format off
#include <cstddef>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// clang-format on

#include <gsl/gsl-lite.hpp>

auto surge::renderer::line::create(GLuint line_shader, const line_data_buffer &buffer) noexcept
    -> context {
  log_info("Creating line buffers");

  const auto num_points{buffer.size() / 3};

  GLuint VAO{0};
  GLuint VBO{0};

  glGenBuffers(1, &VBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, gsl::narrow_cast<GLsizeiptr>(buffer.size() * sizeof(float)),
               buffer.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return context{line_shader, VAO, VBO, num_points};
}

auto surge::renderer::line::create(GLuint line_shader, glm::vec3 &&initial,
                                   glm::vec3 &&final) noexcept -> context {
  log_info("Creating line buffers");

  GLuint VAO{0};
  GLuint VBO{0};

  glGenBuffers(1, &VBO);
  glGenVertexArrays(1, &VAO);

  // clang-format off
  const std::array<float, 20> buffer{
      initial[0], initial[1], initial[2],
      final[0]  , final[1]  , final[2]  ,
  };
  // clang-format on

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, gsl::narrow_cast<GLsizeiptr>(buffer.size() * sizeof(float)),
               buffer.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return context{line_shader, VAO, VBO, 2};
}

auto surge::renderer::line::create(const line_data_buffer &buffer) noexcept
    -> tl::expected<context, error> {

  log_info("Creating line shader");

  const auto line_shader{create_shader_program("shaders/line.vert", "shaders/line.frag")};
  if (!line_shader) {
    log_error("Unable to create line shader");
    return tl::unexpected(error::shader_creation);
  }

  return create(*line_shader, buffer);
}

auto surge::renderer::line::create(glm::vec3 &&initial, glm::vec3 &&final) noexcept
    -> tl::expected<context, error> {
  log_info("Creating line shader");

  const auto line_shader{create_shader_program("shaders/line.vert", "shaders/line.frag")};
  if (!line_shader) {
    log_error("Unable to create line shader");
    return tl::unexpected(error::shader_creation);
  }

  return create(*line_shader, std::move(initial), std::move(final));
}

void surge::renderer::line::draw(const context &ctx, const draw_context &dctx) noexcept {
  glUseProgram(ctx.shader_program);

  uniforms::set(ctx.shader_program, "projection", dctx.projection);
  uniforms::set(ctx.shader_program, "view", dctx.view);

  glBindVertexArray(ctx.VAO);
  glDrawArrays(GL_LINE_STRIP, 0, gsl::narrow_cast<GLsizei>(ctx.num_points));

  glBindVertexArray(0);
}

void surge::renderer::line::draw(const context &ctx, draw_context &&dctx) noexcept {
  draw(ctx, dctx);
}