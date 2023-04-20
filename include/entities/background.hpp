#ifndef SURGE_BACKGROUND_HPP
#define SURGE_BACKGROUND_HPP

#include "allocator.hpp"
#include "sad_file.hpp"

// clang-format off
#include "opengl/headers.hpp"
// clang-format on

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace surge {

class background {
public:
  background(const std::filesystem::path &sprite_set_path,
             const char *sprite_sheet_ext = ".png") noexcept;

  void draw() noexcept;

private:
  // OpenGL Buffers and texture containing the sprite sheet
  const GLuint VAO{0}, VBO{0}, EBO{0};

  glm::mat4 model_matrix{1.0f};

  // Data and corresponding OpenGL texture for the spriteset
  const struct spriteset_data {
    glm::vec2 set_dimentions{0.0f};
    GLuint gl_texture_idx{0};
  } spriteset{};

  // Generate OpenGL Buffers
  [[nodiscard]] auto gen_buff() const noexcept -> GLuint;
  [[nodiscard]] auto gen_vao() const noexcept -> GLuint;

  [[nodiscard]] auto load_spriteset(const std::filesystem::path &p, const char *ext) const noexcept
      -> spriteset_data;

  void create_quad() noexcept;
};

} // namespace surge

#endif // SURGE_BACKGROUND_HPP