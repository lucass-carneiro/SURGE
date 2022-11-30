#include "mesh/sprite.hpp"

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

void surge::sprite::draw(GLuint shader_program, const glm::mat4 &projection,
                         const glm::mat4 &view) const noexcept {

  glUseProgram(shader_program);
  set_uniform(shader_program, "txt_0", GLint{0});
  set_uniform(shader_program, "projection", projection);
  set_uniform(shader_program, "view", view);
  set_uniform(shader_program, "model", model_matrix);
  set_uniform(shader_program, "sheet_coords", sheet_coords);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  glBindVertexArray(0);
}

void surge::sprite::sheet_reset() noexcept {
  sheet_coords[0] = -1;
  sheet_coords[1] = -1;
}

void surge::sprite::sheet_set(int i, int j) noexcept {
  sheet_coords[0] = i;
  sheet_coords[1] = j;
}

void surge::sprite::sheet_next() noexcept {
  const int current_i{sheet_coords[0]};
  const int current_j{sheet_coords[1]};

  if (current_i < 0 || current_j < 0) {
    sheet_coords[0] = 0;
    sheet_coords[1] = 0;
  } else {
    if ((current_j + 1) < sheet_width) {
      sheet_coords[1] = current_j + 1;
    } else {
      sheet_coords[0] = (current_i + 1) % sheet_heigth;
      sheet_coords[1] = 0;
    }
  }
}

void surge::sprite::move(GLuint shader_program, glm::vec3 &&vec) noexcept {
  model_matrix = glm::translate(model_matrix, vec);
  set_uniform(shader_program, "model", model_matrix);
}

void surge::sprite::scale(GLuint shader_program, glm::vec3 &&vec) noexcept {
  model_matrix = glm::scale(model_matrix, vec);
  set_uniform(shader_program, "model", model_matrix);
}