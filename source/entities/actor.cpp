#include "entities/actor.hpp"

#include "allocator.hpp"
#include "safe_ops.hpp"
#include "window.hpp"

#include <cmath>
#include <numbers>

surge::actor::actor(const std::filesystem::path &sprite_sheet_path,
                    const std::filesystem::path &sad_file_path, std::uint32_t first_anim_idx,
                    glm::vec3 &&anchor, glm::vec3 &&position, glm::vec3 &&scale,
                    const char *sprite_sheet_ext) noexcept
    : actor_sprite{sprite_sheet_path, sprite_sheet_ext, buffer_usage_hint::static_draw},
      sad_file{load_sad_file(sad_file_path)} {

  // Add the animation 0 to the animation queue and set it as the current sprite
  if (sad_file.has_value()) {
    const auto animation{get_animation(sad_file.value(), first_anim_idx)};
    if (animation.has_value()) {
      current_animation = *animation;
      activate_current_animation();
    } else {
      glog<log_event::error>("Unable to recover animation index {} from sad file {}",
                             first_anim_idx, sad_file_path.c_str());
    }
  } else {
    glog<log_event::error>("Unable to load sad file {}", sad_file_path.c_str());
  }

  set_geometry(std::forward<glm::vec3>(anchor), std::forward<glm::vec3>(position),
               std::forward<glm::vec3>(scale));
}

void surge::actor::drop_sad_file() noexcept {
  if (sad_file.has_value()) {
    mi_free(sad_file->data());
  }
  sad_file = load_file_return_t{};
}

void surge::actor::draw() const noexcept {
  actor_sprite.draw(global_engine_window::get().get_shader_program());
};

void surge::actor::move(glm::vec3 &&vec) noexcept {
  current_quad.corner += vec;
  actor_sprite.set_geometry(global_engine_window::get().get_shader_program(),
                            std::forward<glm::vec3>(current_quad.corner),
                            std::forward<glm::vec3>(current_quad.dims));
}

void surge::actor::scale(glm::vec3 &&vec) noexcept {
  const glm::vec3 position = current_quad.corner + current_quad.anchor;

  current_quad.anchor = current_quad.anchor * vec;
  current_quad.dims = current_quad.dims * vec;
  current_quad.corner = position - current_quad.anchor;

  actor_sprite.set_geometry(global_engine_window::get().get_shader_program(),
                            std::forward<glm::vec3>(current_quad.corner),
                            std::forward<glm::vec3>(current_quad.dims));
}

void surge::actor::set_geometry(glm::vec3 &&anchor, glm::vec3 &&position,
                                glm::vec3 &&scale) noexcept {

  current_quad.anchor = anchor * scale;

  current_quad.dims = glm::vec3{static_cast<float>(current_animation.Sw) * scale[0],
                                static_cast<float>(current_animation.Sh) * scale[1], 0.0};

  current_quad.corner = position - current_quad.anchor;

  actor_sprite.set_geometry(global_engine_window::get().get_shader_program(),
                            std::forward<glm::vec3>(current_quad.corner),
                            std::forward<glm::vec3>(current_quad.dims));
}

void surge::actor::toggle_h_flip() noexcept {
  actor_sprite.toggle_h_flip(global_engine_window::get().get_shader_program());
  current_animation_h_flipped = !current_animation_h_flipped;
}

void surge::actor::toggle_v_flip() noexcept {
  actor_sprite.toggle_v_flip(global_engine_window::get().get_shader_program());
  current_animation_v_flipped = !current_animation_v_flipped;
}

[[nodiscard]] auto surge::actor::get_anchor_coords() const noexcept -> glm::vec3 {
  return current_quad.corner + current_quad.anchor;
}

[[nodiscard]] auto surge::actor::compute_heading(const glm::vec3 &displacement) const noexcept
    -> actor_heading {

  const auto phi{std::numbers::pi + std::atan2(displacement[1], displacement[0])};
  const auto dphi{std::numbers::pi / 8};

  if (dphi < phi && phi < 3 * dphi) {
    return actor_heading::north_west;
  } else if (3 * dphi < phi && phi < 5 * dphi) {
    return actor_heading::north;
  } else if (5 * dphi < phi && phi < 7 * dphi) {
    return actor_heading::north_east;
  } else if (7 * dphi < phi && phi < 9 * dphi) {
    return actor_heading::east;
  } else if (9 * dphi < phi && phi < 11 * dphi) {
    return actor_heading::south_east;
  } else if (11 * dphi < phi && phi < 13 * dphi) {
    return actor_heading::south;
  } else if (13 * dphi < phi && phi < 15 * dphi) {
    return actor_heading::south_west;
  } else {
    return actor_heading::west;
  }
}

void surge::actor::swtich_to_animation(std::uint32_t idx) noexcept {
  if (sad_file.has_value()) {
    const auto animation{get_animation(sad_file.value(), idx)};
    if (animation.has_value()) {
      current_animation = *animation;
      current_alpha = 0;
      current_beta = 0;
      activate_current_animation();
    } else {
      glog<log_event::error>("Unable to recover animation index {} from sad file", idx);
    }
  } else {
    glog<log_event::error>("Unable to load sad file");
  }
}

void surge::actor::activate_current_animation() noexcept {
  actor_sprite.sheet_set_offset(glm::ivec2{current_animation.x, current_animation.y});
  actor_sprite.sheet_set_dimentions(glm::ivec2{current_animation.Sw, current_animation.Sh});
  actor_sprite.sheet_set_indices(glm::ivec2{current_alpha, current_beta});
}

void surge::actor::advance_current_anim_frame() noexcept {
  if ((current_beta + 1) < current_animation.cols) {
    current_beta = current_beta + 1;
  } else {
    current_alpha = (current_alpha + 1) % current_animation.rows;
    current_beta = 0;
  }

  actor_sprite.sheet_set_indices(glm::ivec2{current_alpha, current_beta});
}

void surge::actor::update_animations(double animation_frame_dt) noexcept {
  // Handle animations
  static double frame_time{0};
  frame_time = frame_time + global_engine_window::get().get_frame_dt();

  if (frame_time > animation_frame_dt) {
    frame_time = 0.0;
    advance_current_anim_frame();
  }
}

constexpr static const auto heading_north{static_cast<lua_Integer>(surge::actor_heading::north)};
constexpr static const auto heading_south{static_cast<lua_Integer>(surge::actor_heading::south)};
constexpr static const auto heading_east{static_cast<lua_Integer>(surge::actor_heading::east)};
constexpr static const auto heading_west{static_cast<lua_Integer>(surge::actor_heading::west)};
constexpr static const auto heading_north_east{
    static_cast<lua_Integer>(surge::actor_heading::north_east)};
constexpr static const auto heading_north_west{
    static_cast<lua_Integer>(surge::actor_heading::north_west)};
constexpr static const auto heading_south_east{
    static_cast<lua_Integer>(surge::actor_heading::south_east)};
constexpr static const auto heading_south_west{
    static_cast<lua_Integer>(surge::actor_heading::south_west)};

void surge::actor::walk_to(glm::vec3 &&target, float speed, float threshold) noexcept {
  const auto current_anchor{get_anchor_coords()};
  const auto current_displacement{target - current_anchor};
  const auto current_displacement_length{glm::length(current_displacement)};

  if (current_displacement_length > threshold) {
    const auto normalized_displacement{glm::normalize(current_displacement)};
    const auto heading_direction{static_cast<lua_Integer>(compute_heading(current_displacement))};

    switch (heading_direction) {

    case heading_north:
      if (current_animation.index != 0)
        swtich_to_animation(0);
      break;

    case heading_south:
      if (current_animation.index != 1)
        swtich_to_animation(1);
      break;

    case heading_east:
      if (current_animation.index != 6) {
        swtich_to_animation(6);
      }
      if (current_animation_h_flipped) {
        toggle_h_flip();
      }
      break;

    case heading_west:
      if (current_animation.index != 6) {
        swtich_to_animation(6);
      }
      if (!current_animation_h_flipped) {
        toggle_h_flip();
      }
      break;

    case heading_north_east:
      if (current_animation.index != 6) {
        swtich_to_animation(6);
      }
      if (current_animation_h_flipped) {
        toggle_h_flip();
      }
      break;

    case heading_north_west:
      if (current_animation.index != 6) {
        swtich_to_animation(6);
      }
      if (!current_animation_h_flipped) {
        toggle_h_flip();
      }
      break;

    case heading_south_east:
      if (current_animation.index != 6) {
        swtich_to_animation(6);
      }
      if (current_animation_h_flipped) {
        toggle_h_flip();
      }
      break;

    case heading_south_west:
      if (current_animation.index != 6) {
        swtich_to_animation(6);
      }
      if (!current_animation_h_flipped) {
        toggle_h_flip();
      }
      break;

    default:
      break;
    }
    move(speed * normalized_displacement);

  } else {

    const auto heading_direction{static_cast<lua_Integer>(compute_heading(current_displacement))};

    switch (heading_direction) {

    case heading_north:
      swtich_to_animation(3);
      break;

    case heading_south:
      swtich_to_animation(4);
      break;

    case heading_east:
      swtich_to_animation(5);
      break;

    case heading_west:
      swtich_to_animation(5);
      break;

    case heading_north_east:
      swtich_to_animation(5);
      break;

    case heading_north_west:
      swtich_to_animation(5);
      break;

    case heading_south_east:
      swtich_to_animation(5);
      break;

    case heading_south_west:
      swtich_to_animation(5);
      break;

    default:
      break;
    }
  }
}