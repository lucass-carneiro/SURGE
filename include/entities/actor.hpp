#ifndef SURGE_ACTOR_HPP
#define SURGE_ACTOR_HPP

#include "allocator.hpp"
#include "sad_file.hpp"
#include "sprite.hpp"

#include <deque>
#include <queue>

namespace surge {

class actor {
public:
  actor(const std::filesystem::path &sprite_set_path, const std::filesystem::path &sad_file_path,
        std::uint32_t first_anim_idx, glm::vec3 &&anchor, glm::vec3 &&position, glm::vec3 &&scale,
        const char *sprite_sheet_ext = ".png") noexcept;

  void draw() noexcept;
  void update(double frame_update_delay) noexcept;

  void move(glm::vec3 &&vec) noexcept;
  void scale(glm::vec3 &&vec) noexcept;

  void change_current_animation_to(std::uint32_t index, bool loops = true) noexcept;

private:
  // OpenGL Buffers and texture containing the sprite sheet
  const GLuint VAO{0}, VBO{0}, EBO{0};

  glm::mat4 model_matrix{1.0f};

  struct actor_quad_info {
    glm::vec3 corner{0.0f};
    glm::vec3 dims{0.0f};
    glm::vec3 anchor{0.0f}; // Holds pixel coordinates
  } current_quad{};

  // Data and corresponding OpenGL texture for the spriteset
  const struct spriteset_data {
    glm::vec2 set_dimentions{0.0f};
    GLuint gl_texture_idx{0};
  } spriteset{};

  // The associated animation data file
  const std::optional<sad_file_contents> sad_file{};

  struct animation_data {
    std::uint32_t animation_index{0};
    std::uint32_t linearized_animation_frame_index{0};
    std::uint32_t spritesheet_size{0};
    bool loops{true};
    bool h_flip{false};
    bool v_flip{false};
  } current_animation_data{};

  // Generate OpenGL Buffers
  [[nodiscard]] auto gen_buff() const noexcept -> GLuint;
  [[nodiscard]] auto gen_vao() const noexcept -> GLuint;

  [[nodiscard]] auto load_spriteset(const std::filesystem::path &p, const char *ext) const noexcept
      -> spriteset_data;

  void create_quad() noexcept;

  void reset_geometry(const glm::vec3 &anchor, const glm::vec3 &position,
                      const glm::vec3 &scale) noexcept;
  void reset_geometry(glm::vec3 &&anchor, glm::vec3 &&position, glm::vec3 &&scale) noexcept;

  void toggle_h_flip() noexcept;
  void toggle_v_flip() noexcept;

  [[nodiscard]] auto delinearize_animation_frame_index() const noexcept -> glm::vec2;

  void update_animation_frame() noexcept;
};

} // namespace surge

#endif // SURGE_ACTOR_HPP