#ifndef SURGE_PRIMITIVE_DRAWING_HPP
#define SURGE_PRIMITIVE_DRAWING_HPP

#include "headers.hpp"

#include <array>
#include <concepts>
#include <glm/vec3.hpp>

namespace surge {

struct triangle {
  std::array<float, 9> data{};

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  constexpr inline triangle(const glm::vec3 &p1, const glm::vec3 &p2,
                            const glm::vec3 &p3) noexcept {
    data[0] = p1[0];
    data[1] = p1[1];
    data[2] = p1[2];

    data[3] = p2[0];
    data[4] = p2[1];
    data[5] = p2[2];

    data[6] = p3[0];
    data[7] = p3[1];
    data[8] = p3[2];
  }
};

void send_to_gpu(GLuint VBO, GLuint VAO, const triangle &vertex) noexcept;

} // namespace surge

#endif // SURGE_PRIMITIVE_DRAWING_HPP