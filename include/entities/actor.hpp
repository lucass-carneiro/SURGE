#ifndef SURGE_ACTOR_HPP
#define SURGE_ACTOR_HPP

#include "sad_file.hpp"
#include "sprite.hpp"

namespace surge {

class actor {
public:
  template <surge_allocator alloc_t>
  actor(alloc_t *allocator, const std::filesystem::path &sprite_sheet_path,
        const std::filesystem::path &sad_file_path, const char *sprite_sheet_ext = ".png") noexcept
      : actor_sprite{allocator, sprite_sheet_path, sprite_sheet_ext,
                     buffer_usage_hint::static_draw},
        sad_file{load_sad_file(allocator, sad_file_path)} {}

  template <surge_allocator alloc_t> void drop_sad_file(alloc_t *allocator) noexcept {
    if (sad_file.has_value()) {
      allocator->free(sad_file->data());
    }
    sad_file = load_file_return_t{};
  }

  // void sheet_set_offset(glm::ivec2 &&offset) noexcept;
  // void sheet_set_dimentions(glm::ivec2 &&dimentions) noexcept;
  // void sheet_set_indices(glm::vec2 &&indices) noexcept;

  void draw() const noexcept;

  void set_anchor_point(glm::vec3 &&anchor) noexcept;

  void select_animation(std::uint32_t index) noexcept;
  void advance_frame() noexcept;

  void move(glm::vec3 &&vec) noexcept;
  void scale(glm::vec3 &&vec) noexcept;

  void set_geometry(glm::vec3 &&position, glm::vec3 &&scale) noexcept;
  void set_position(glm::vec3 &&position, float scale) noexcept;

private:
  sprite actor_sprite;
  load_file_return_t sad_file;

  glm::vec3 anchor_point{};

  animation_data current_animation{};
  glm::ivec2 current_frame_indices{};

  void set_zero_animation() noexcept;
};

} // namespace surge

#endif