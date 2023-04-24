#ifndef SURGE_IMAGE_ENTITY_HPP
#define SURGE_IMAGE_ENTITY_HPP

#include "allocator.hpp"
#include "sad_file.hpp"

// clang-format off
#include "opengl/headers.hpp"
// clang-format on

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace surge {

class image_entity {
public:
  image_entity(const std::filesystem::path &sprite_set_path, glm::vec3 &&position,
               glm::vec3 &&scale, const char *sprite_sheet_ext = ".png") noexcept;

  void draw() noexcept;

  void toggle_h_flip() noexcept;
  void toggle_v_flip() noexcept;

private:
  // OpenGL Buffers and texture containing the sprite sheet
  const GLuint VAO{0}, VBO{0}, EBO{0};

  glm::mat4 model_matrix{1.0f};

  // Data and corresponding OpenGL texture for the spriteset
  const struct texture_data {
    glm::vec2 dimentions{0.0f};
    GLuint gl_texture_idx{0};
  } texture{};

  bool current_h_flip{false};
  bool current_v_flip{false};

  // Generate OpenGL Buffers
  [[nodiscard]] auto gen_buff() const noexcept -> GLuint;
  [[nodiscard]] auto gen_vao() const noexcept -> GLuint;

  [[nodiscard]] auto load_texture(const std::filesystem::path &p, const char *ext) const noexcept
      -> texture_data;

  void create_quad(glm::vec3 &position, glm::vec3 &scale) noexcept;
};

} // namespace surge

#endif // SURGE_IMAGE_ENTITY_HPP