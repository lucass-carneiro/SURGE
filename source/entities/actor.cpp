#include "entities/actor.hpp"

#include "window.hpp"

#include <cmath>
#include <numbers>

void surge::actor::draw() const noexcept {
  actor_sprite.draw(global_engine_window::get().get_shader_program());
};

void surge::actor::set_zero_animation() noexcept {
  current_animation = animation_data{};
  actor_sprite.sheet_set_offset(glm::ivec2{0, 0});
  actor_sprite.sheet_set_dimentions(glm::ivec2{0, 0});
  actor_sprite.sheet_set_indices(glm::ivec2{0, 0});
}

void surge::actor::select_animation(std::uint32_t index) noexcept {
  if (!current_animation_playing) {
    if (sad_file.has_value()) {
      const auto animation{get_animation(sad_file.value(), index)};

      if (animation.has_value()) {
        actor_sprite.sheet_set_offset(glm::ivec2{animation->x, animation->y});
        actor_sprite.sheet_set_dimentions(glm::ivec2{animation->Sw, animation->Sh});
        actor_sprite.sheet_set_indices(glm::ivec2{0, 0});
        current_animation = animation.value();
        current_animation_playing = true;

      } else {
        glog<log_event::error>("Unable retrieve aniation index {}.", index);
        set_zero_animation();
      }
    } else {
      glog<log_event::error>("Unable to set animation because .sad file was not loaded correctly.");
      set_zero_animation();
    }
  }
}

void surge::actor::advance_frame() noexcept {
  if (current_frame_indices[0] < 0 || current_frame_indices[1] < 0) {
    current_frame_indices[0] = 0;
    current_frame_indices[1] = 0;
  } else if ((current_frame_indices[1] + 1) < static_cast<int>(current_animation.cols)) {
    current_frame_indices[1] = current_frame_indices[1] + 1;
  } else {
    current_frame_indices[0]
        = (current_frame_indices[0] + 1) % static_cast<int>(current_animation.rows);
    current_frame_indices[1] = 0;
  }

  actor_sprite.sheet_set_indices(current_frame_indices);
}

void surge::actor::advance_frame(glm::ivec2 &&final_frame) noexcept {
  if (current_frame_indices[0] < 0 || current_frame_indices[1] < 0) {
    current_frame_indices[0] = 0;
    current_frame_indices[1] = 0;
  } else if ((current_frame_indices[1] + 1) < static_cast<int>(current_animation.cols)) {
    current_frame_indices[1] = current_frame_indices[1] + 1;
  } else if ((current_frame_indices[0] + 1) < static_cast<int>(current_animation.rows)) {
    current_frame_indices[0]
        = (current_frame_indices[0] + 1) % static_cast<int>(current_animation.rows);
    current_frame_indices[1] = 0;
  } else {
    current_frame_indices = final_frame;
  }

  actor_sprite.sheet_set_indices(current_frame_indices);
}

void surge::actor::play_animation(double animation_frame_dt) noexcept {
  static double frame_time{0};
  frame_time = frame_time + global_engine_window::get().get_frame_dt();

  if (frame_time > animation_frame_dt) {
    frame_time = 0.0;
    // advance_frame();
    advance_frame(glm::ivec2{0, 0});
  }
}

void surge::actor::move(glm::vec3 &&vec) noexcept {
  current_quad.corner = current_quad.corner + vec;

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
}

void surge::actor::toggle_v_flip() noexcept {
  actor_sprite.toggle_v_flip(global_engine_window::get().get_shader_program());
}

[[nodiscard]] auto surge::actor::get_anchor_coords() const noexcept -> glm::vec3 {
  return current_quad.corner + current_quad.anchor;
}

[[nodiscard]] auto surge::actor::compute_heading(glm::vec3 &&target) const noexcept
    -> actor_heading {
  const auto anchor_coords{get_anchor_coords()};
  const auto displacement{target - anchor_coords};
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