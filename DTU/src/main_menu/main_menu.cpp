#include "DTU.hpp"
#include "container_types.hpp"

// clang-format off
#include "main_menu/main_menu.hpp"
#include "error_types.hpp"

#include "player/logging.hpp"
#include "player/static_image.hpp"
// clang-format on

#include <omp.h>

auto DTU::state::main_menu::state_load() noexcept -> std::optional<DTU::error> {
  using namespace surge;

  log_info("Loading main_menu state");

  // Load background
  log_info("Loading background images");
  DTU::array<atom::static_image::load_image_t, 5> image_data{};

#pragma omp parallel num_threads(4)
  {
    image_data[0] = atom::static_image::load_image("resources/main_menu/00.png");
    image_data[1] = atom::static_image::load_image("resources/main_menu/01.png");
    image_data[2] = atom::static_image::load_image("resources/main_menu/02.png");
    image_data[3] = atom::static_image::load_image("resources/main_menu/03.png");
    image_data[4] = atom::static_image::load_image("resources/main_menu/04.png");
  }

  for (auto &img : image_data) {
    if (!img) {
      return DTU::error::image_load;
    } else {
      const auto texture{atom::static_image::make_texture(*img)};
      components::static_image_buffer::dimentions().push_back(texture.dimentions);
      components::static_image_buffer::ds().push_back(texture.ds);
      components::static_image_buffer::texture_id().push_back(texture.texture_id);
      components::static_image_buffer::VBO().push_back(texture.VBO);
      components::static_image_buffer::EBO().push_back(texture.EBO);
      components::static_image_buffer::VAO().push_back(texture.VAO);
    }
  }

  return {};
}

auto DTU::state::main_menu::state_unload() noexcept -> std::optional<DTU::error> {
  log_info("Unloading main_menu state");

  components::static_image_buffer::clean_and_reset();

  return {};
}