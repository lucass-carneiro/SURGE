#ifndef SURGE_SPRITE_HPP
#define SURGE_SPRITE_HPP

#include "allocators/allocators.hpp"
#include "opengl/buffer_usage_hints.hpp"
#include "opengl/gl_uniforms.hpp"
#include "opengl/headers.hpp"
#include "opengl/load_texture.hpp"

#include <array>
#include <filesystem>

namespace surge {

struct sprite_position {
  float x{0};
  float y{0};
  float z{0};
  float width{0};
  float height{0};
};

/**
 * @brief A sprite is a 2D quad (rectangle) with a diffuse map.
 *
 */
class sprite {
public:
  template <surge_allocator alloc_t>
  sprite(alloc_t *allocator, const std::filesystem::path &p, const char *ext,
         buffer_usage_hint usage_hint, const sprite_position &pos) noexcept
      : VAO{gen_vao()},
        VBO{gen_buff()},
        EBO{gen_buff()},
        texture_id{load_texture(allocator, p, ext).value_or(0)} {

    // clang-format off
    std::array<float, 20> vertex_attributes{
        pos.x            , pos.y             , pos.z, 0.0f, 1.0f, // top left
        pos.x            , pos.y + pos.height, pos.z, 0.0f, 0.0f, // bottom left
        pos.x + pos.width, pos.y + pos.height, pos.z, 1.0f, 0.0f, // bottom right
        pos.x + pos.width, pos.y             , pos.z, 1.0f, 1.0f, // top right
    };
    // clang-format on

    std::array<GLuint, 6> draw_indices{1, 2, 3, 3, 0, 1};

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float),
                 vertex_attributes.data(), to_gl_hint(usage_hint));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
                 to_gl_hint(usage_hint));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<const void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  void draw(GLuint shader_program, const glm::mat4 &projection,
            const glm::mat4 &view) const noexcept {

    // TODO: The user sets the model matrix
    const auto model{glm::mat4(1.0f)};

    glUseProgram(shader_program);
    set_uniform(shader_program, "txt_0", GLint{0});
    set_uniform(shader_program, "projection", projection);
    set_uniform(shader_program, "view", view);
    set_uniform(shader_program, "model", model);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
  }

private:
  const GLuint VAO{0}, VBO{0}, EBO{0}, texture_id{0};

  [[nodiscard]] inline auto gen_buff() const noexcept -> GLuint {
    GLuint tmp{0};
    glGenBuffers(1, &tmp);
    return tmp;
  }

  [[nodiscard]] inline auto gen_vao() const noexcept -> GLuint {
    GLuint tmp{0};
    glGenVertexArrays(1, &tmp);
    return tmp;
  }
};

} // namespace surge

#endif // SURGE_SPRITE_HPP