#include "opengl/primitive_drawing.hpp"

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void surge::send_to_gpu(GLuint VBO, GLuint VAO, const triangle &vertex) noexcept {
  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertex.data.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}