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

  [[nodiscard]] auto compute_heading(const glm::vec3 &target) const noexcept -> actor_heading;

  void push_anim_to_queue(const animation_data &data) noexcept;
  void pop_anim_from_queue() noexcept;
  void set_front_anim_as_current() noexcept;
  void advance_current_anim_frame() noexcept;

  void update_animations(double animation_frame_dt) noexcept;
  void walk_to(glm::vec3 &&target, float speed, float threshold) noexcept;

private:
  sprite actor_sprite;
  load_file_return_t sad_file;

  actor_quad_info current_quad{};

  using alloc_t = mi_stl_allocator<std::uint32_t>;
  using deque_t = std::deque<std::uint32_t, alloc_t>;
  using anim_data_queue = std::queue<std::uint32_t, deque_t>;

  anim_data_queue anim_queue_index;
  anim_data_queue anim_queue_x;
  anim_data_queue anim_queue_y;
  anim_data_queue anim_queue_Sw;
  anim_data_queue anim_queue_Sh;
  anim_data_queue anim_queue_rows;
  anim_data_queue anim_queue_cols;

  std::uint32_t current_alpha{0};
  std::uint32_t current_beta{0};

  void set_zero_animation() noexcept;
};

} // namespace surge

#endif