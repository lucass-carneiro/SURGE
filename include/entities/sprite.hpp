#ifndef SURGE_SPRITE_HPP
#define SURGE_SPRITE_HPP

#include "allocators/allocators.hpp"
#include "opengl/buffer_usage_hints.hpp"
#include "opengl/gl_uniforms.hpp"
#include "opengl/headers.hpp"
#include "opengl/load_texture.hpp"

#include <filesystem>

namespace surge {

template <std::size_t i, typename T> [[nodiscard]] auto buffer_offset() noexcept -> const void * {
  // NOLINTNEXTLINE
  return reinterpret_cast<const void *>(i * sizeof(T));
}

/**
 * @brief A sprite is a 2D quad (rectangle) with a diffuse map.
 * see https://gamedev.stackexchange.com/q/170083
 *
 */
class sprite {
public:
  template <surge_allocator alloc_t> sprite(alloc_t *allocator, const std::filesystem::path &p,
                                            const char *ext, buffer_usage_hint usage_hint) noexcept
      : VAO{gen_vao()},
        VBO{gen_buff()},
        EBO{gen_buff()},
        texture_id{load_texture(allocator, p, ext).value_or(0)},
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
    glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float),
                 vertex_attributes.data(), to_gl_hint(usage_hint));

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

  void draw(GLuint shader_program) const noexcept;

  void sheet_set_offset(glm::ivec2 &&offset) noexcept;
  void sheet_set_dimentions(glm::ivec2 &&dimentions) noexcept;
  void sheet_set_indices(glm::vec2 &&indices) noexcept;

  void move(GLuint shader_program, glm::vec3 &&vec) noexcept;
  void scale(GLuint shader_program, glm::vec3 &&vec) noexcept;
  void set_geometry(GLuint shader_program, glm::vec3 &&position, glm::vec3 &&scale) noexcept;

  void toggle_h_flip(GLuint shader_program) noexcept;
  void toggle_v_flip(GLuint shader_program) noexcept;

private:
  const GLuint VAO{0}, VBO{0}, EBO{0}, texture_id{0};
  const glm::vec2 set_dimentions{0.0f, 0.0f};

  glm::mat4 model_matrix{1.0f};

  glm::vec2 sheet_offsets{0.0f, 0.0f};
  glm::vec2 sheet_dimentions{0.0f, 0.0f};
  glm::vec2 sheet_indices{0.0f, 0.0f};

  bool sheet_h_flip{false}, sheet_v_flip{false};

  [[nodiscard]] auto gen_buff() const noexcept -> GLuint;
  [[nodiscard]] auto gen_vao() const noexcept -> GLuint;
  [[nodiscard]] auto dimentions_from_texture() -> glm::vec2;
};

} // namespace surge

#endif // SURGE_SPRITE_HPP