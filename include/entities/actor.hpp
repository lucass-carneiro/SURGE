#ifndef SURGE_ACTOR_HPP
#define SURGE_ACTOR_HPP

#include "allocator.hpp"
#include "sad_file.hpp"
#include "sprite.hpp"

#include <deque>
#include <queue>

namespace surge {

struct actor_quad_info {
  glm::vec3 corner{0.0f};
  glm::vec3 dims{0.0f};
  glm::vec3 anchor{0.0f}; // Holds pixel coordinates
};

enum class actor_heading : std::ptrdiff_t {
  north = 0,
  south = 1,
  east = 2,
  west = 3,
  north_east = 4,
  north_west = 5,
  south_east = 6,
  south_west = 7,
  none = 8
};

class actor {
public:
  actor(const std::filesystem::path &sprite_sheet_path, const std::filesystem::path &sad_file_path,
        std::uint32_t first_anim_idx, glm::vec3 &&anchor, glm::vec3 &&position, glm::vec3 &&scale,
        const char *sprite_sheet_ext = ".png") noexcept;

  void drop_sad_file() noexcept;

  void draw() const noexcept;

  void move(glm::vec3 &&vec) noexcept;
  void scale(glm::vec3 &&vec) noexcept;

  void set_geometry(glm::vec3 &&anchor, glm::vec3 &&position, glm::vec3 &&scale) noexcept;

  void toggle_h_flip() noexcept;
  void toggle_v_flip() noexcept;

  [[nodiscard]] auto get_anchor_coords() const noexcept -> glm::vec3;

  [[nodiscard]] auto compute_heading(const glm::vec3 &displacement) const noexcept -> actor_heading;

  void swtich_to_animation(std::uint32_t idx) noexcept;
  void activate_current_animation() noexcept;
  void advance_current_anim_frame() noexcept;

  void update_animations(double animation_frame_dt) noexcept;
  void walk_to(glm::vec3 &&target, float speed, float threshold) noexcept;

private:
  sprite actor_sprite;
  load_file_return_t sad_file;

  actor_quad_info current_quad{};

  animation_data current_animation{};
  bool current_animation_h_flipped{false};
  bool current_animation_v_flipped{false};

  std::uint32_t current_alpha{0};
  std::uint32_t current_beta{0};

  void set_zero_animation() noexcept;
};

} // namespace surge

#endif