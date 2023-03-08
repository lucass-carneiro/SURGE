#ifndef SURGE_ACTOR_HPP
#define SURGE_ACTOR_HPP

#include "allocators/global_allocators.hpp"
#include "allocators/stl_allocator.hpp"
#include "sad_file.hpp"
#include "sprite.hpp"

namespace surge {

struct actor_quad_info {
  glm::vec3 corner{0.0f};
  glm::vec3 dims{0.0f};
  glm::vec3 anchor{0.0f};
};

enum class actor_heading : std::ptrdiff_t {
  north = 0,
  south = 1,
  east = 2,
  west = 3,
  north_east = 4,
  north_west = 5,
  south_east = 6,
  south_west = 7
};

class actor {
public:
  template <surge_allocator alloc_t>
  actor(alloc_t *allocator, const std::filesystem::path &sprite_sheet_path,
        const std::filesystem::path &sad_file_path, const char *sprite_sheet_ext = ".png") noexcept
      : actor_sprite{allocator, sprite_sheet_path, sprite_sheet_ext,
                     buffer_usage_hint::static_draw},
        sad_file{load_sad_file(allocator, sad_file_path)} {
    select_animation(0);
  }

  template <surge_allocator alloc_t> void drop_sad_file(alloc_t *allocator) noexcept {
    if (sad_file.has_value()) {
      allocator->free(sad_file->data());
    }
    sad_file = load_file_return_t{};
  }

  void draw() const noexcept;

  void select_animation(std::uint32_t index) noexcept;
  void advance_frame() noexcept;
  void advance_frame(glm::ivec2 &&final_frame) noexcept;
  void play_animation(double animation_frame_dt) noexcept;

  void move(glm::vec3 &&vec) noexcept;
  void scale(glm::vec3 &&vec) noexcept;

  void set_geometry(glm::vec3 &&anchor, glm::vec3 &&position, glm::vec3 &&scale) noexcept;

  void toggle_h_flip() noexcept;
  void toggle_v_flip() noexcept;

  [[nodiscard]] auto get_anchor_coords() const noexcept -> glm::vec3;

  [[nodiscard]] auto compute_heading(glm::vec3 &&target) const noexcept -> actor_heading;

private:
  sprite actor_sprite;
  load_file_return_t sad_file;

  actor_quad_info current_quad{};

  actor_heading current_heading{actor_heading::north};

  animation_data current_animation{};
  glm::ivec2 current_frame_indices{};
  bool current_animation_playing{false};

  void set_zero_animation() noexcept;
};

} // namespace surge

#endif