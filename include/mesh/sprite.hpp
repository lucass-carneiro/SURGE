#ifndef SURGE_SPRITE_HPP
#define SURGE_SPRITE_HPP

#include "allocators/allocators.hpp"
#include "opengl/buffer_usage_hints.hpp"
#include "opengl/gl_uniforms.hpp"
#include "opengl/headers.hpp"
#include "opengl/load_texture.hpp"

#include <filesystem>

namespace surge {

/**
 * @brief A sprite is a 2D quad (rectangle) with a diffuse map.
 * see https://gamedev.stackexchange.com/q/170083
 *
 */
class sprite {
public:
  template <surge_allocator alloc_t>
  sprite(alloc_t *allocator, const std::filesystem::path &p, const char *ext, GLint sw, GLint sh,
         buffer_usage_hint usage_hint) noexcept
      : VAO{gen_vao()},
        VBO{gen_buff()},
        EBO{gen_buff()},
        texture_id{load_texture(allocator, p, ext).value_or(0)},
        sheet_width{sw},
        sheet_heigth{sh} {

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
    const auto offset_1{reinterpret_cast<const void *>(3 * sizeof(float))};
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), offset_1);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void draw(GLuint shader_program, const glm::mat4 &projection,
            const glm::mat4 &view) const noexcept;

  void sheet_reset() noexcept;
  void sheet_set(int i, int j) noexcept;
  void sheet_next() noexcept;

  void move(GLuint shader_program, glm::vec3 &&vec) noexcept;
  void scale(GLuint shader_program, glm::vec3 &&vec) noexcept;

private:
  const GLuint VAO{0}, VBO{0}, EBO{0}, texture_id{0};
  const GLint sheet_width{0}, sheet_heigth{0};
  glm::mat4 model_matrix{1.0f};
  glm::ivec4 sheet_coords{-1, -1, sheet_width, sheet_heigth};

  [[nodiscard]] auto gen_buff() const noexcept -> GLuint;
  [[nodiscard]] auto gen_vao() const noexcept -> GLuint;
};

} // namespace surge

#endif // SURGE_SPRITE_HPP