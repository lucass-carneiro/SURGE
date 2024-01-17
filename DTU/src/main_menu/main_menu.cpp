#include "DTU.hpp"

// clang-format off
#include "main_menu/main_menu.hpp"
// clang-format on

#include "player/logging.hpp"
#include "player/static_image.hpp"
#include "renderer.hpp"
#include "static_image.hpp"
#include "window.hpp"

#include <glm/fwd.hpp>
#include <omp.h>

auto DTU::state::main_menu::state_load() noexcept -> int {
  using namespace surge;

  log_info("Loading main_menu state");

  // Load background
  log_info("Loading background images");
  auto img_data{atom::static_image::create("resources/main_menu/background.png",
                                           renderer::texture_filtering::nearest)};

  if (!img_data) {
    return static_cast<int>(img_data.error());
  }

  components::static_image_buffer::get().push_back(*img_data);

  return 0;
}

auto DTU::state::main_menu::state_unload() noexcept -> int {
  log_info("Unloading main_menu state");

  components::static_image_buffer::reset();
  components::static_image_draw::reset();

  return 0;
}

auto DTU::state::main_menu::draw() noexcept -> int {
  // clang-format off
  surge::atom::static_image::one_draw_data draw_data{
    DTU::get_projection(),
    DTU::get_view(),
    glm::vec3{0},
    glm::vec3{800.0, 600.0, 0.0f},
    glm::vec2{0},
    glm::vec2{576.0f, 324.0f},
    false,
    false
  };
  // clang-format on

  const auto &shader{DTU::get_img_shader()};
  const auto &buffer{DTU::components::static_image_buffer::get().back()};

  // Level 0
  surge::atom::static_image::draw(shader, buffer, draw_data);

  return 0;
}

/*
.0 {
   background: url('spritesheet.png') no-repeat -0px -0px;
   width: 1152px;
   height: 324px;
}
.1 {
   background: url('spritesheet.png') no-repeat -0px -326px;
   width: 1152px;
   height: 172px;
}
.2 {
   background: url('spritesheet.png') no-repeat -0px -500px;
   width: 1152px;
   height: 144px;
}
.3 {
   background: url('spritesheet.png') no-repeat -0px -646px;
   width: 1151px;
   height: 141px;
}
.4 {
   background: url('spritesheet.png') no-repeat -0px -789px;
   width: 1152px;
   height: 95px;
}
*/