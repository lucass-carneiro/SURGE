#include "entities/sprite.hpp"

#include <array>

surge::sprite::sprite(const std::filesystem::path &p, const char *ext,
                      buffer_usage_hint usage_hint) noexcept
    : VAO{gen_vao()},
      VBO{gen_buff()},
      EBO{gen_buff()},
      texture_id{load_texture(p, ext).value_or(0)},
      set_dimentions{dimentions_from_texture()} {

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
               to_gl_hint(usage_hint));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               to_gl_hint(usage_hint));

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // NOLINTNEXTLINE
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), buffer_offset<3, float>());
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

auto surge::sprite::gen_buff() const noexcept -> GLuint {
  GLuint tmp{0};
  glGenBuffers(1, &tmp);
  return tmp;
}

auto surge::sprite::gen_vao() const noexcept -> GLuint {
  GLuint tmp{0};
  glGenVertexArrays(1, &tmp);
  return tmp;
}

auto surge::sprite::dimentions_from_texture() -> glm::vec2 {
  GLint w{0}, h{0};

  glBindTexture(GL_TEXTURE_2D, texture_id);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
  glBindTexture(GL_TEXTURE_2D, 0);

  return glm::vec2{static_cast<GLfloat>(w), static_cast<GLfloat>(h)};
}

void surge::sprite::draw(GLuint shader_program) const noexcept {
  set_uniform(shader_program, "txt_0", GLint{0});
  set_uniform(shader_program, "model", model_matrix);

  set_uniform(shader_program, "sheet_set_dimentions", set_dimentions);
  set_uniform(shader_program, "sheet_offsets", sheet_offsets);
  set_uniform(shader_program, "sheet_dimentions", sheet_dimentions);
  set_uniform(shader_program, "sheet_indices", sheet_indices);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::sprite::sheet_set_offset(glm::ivec2 &&offset) noexcept { sheet_offsets = offset; }

void surge::sprite::sheet_set_dimentions(glm::ivec2 &&dimentions) noexcept {
  sheet_dimentions = dimentions;
}

void surge::sprite::sheet_set_indices(glm::vec2 &&indices) noexcept { sheet_indices = indices; }

void surge::sprite::move(GLuint shader_program, glm::vec3 &&vec) noexcept {
  model_matrix = glm::translate(model_matrix, vec);
  set_uniform(shader_program, "model", model_matrix);
}

void surge::sprite::scale(GLuint shader_program, glm::vec3 &&vec) noexcept {
  model_matrix = glm::scale(model_matrix, vec);
  set_uniform(shader_program, "model", model_matrix);
}

void surge::sprite::set_geometry(GLuint shader_program, glm::vec3 &&position,
                                 glm::vec3 &&scale) noexcept {

  model_matrix = glm::mat4{1.0f};
  model_matrix = glm::translate(model_matrix, position);
  model_matrix = glm::scale(model_matrix, scale);

  set_uniform(shader_program, "model", model_matrix);
}

void surge::sprite::toggle_h_flip(GLuint shader_program) noexcept {
  sheet_h_flip = !sheet_h_flip;
  set_uniform(shader_program, "h_flip", static_cast<GLboolean>(sheet_h_flip));
}

void surge::sprite::toggle_v_flip(GLuint shader_program) noexcept {
  sheet_v_flip = !sheet_v_flip;
  set_uniform(shader_program, "v_flip", static_cast<GLboolean>(sheet_v_flip));
}