#include "new_game/new_game.hpp"

#include "player/logging.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

static constexpr const surge::usize char_sheet_img_count{1};

static void load_char_sheet_imgs(surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Loading background images");

  const std::array<const char *, char_sheet_img_count> img_names{
      "resources/new_game/character_sheet.png"};

  // Load images
  for (const auto &image_name : img_names) {
    auto img{files::load_image(image_name)};

    if (img) {
      const auto texture_data{
          atom::sprite::create_texture(*img, renderer::texture_filtering::nearest)};

      if (texture_data) {
        dl.texture_ids.push_back(std::get<0>(*texture_data));
        dl.texture_handles.push_back(std::get<1>(*texture_data));
        dl.alphas.push_back(1.0f);
      } else {
        dl.texture_ids.push_back(0);
        dl.texture_handles.push_back(0);
        dl.alphas.push_back(0.0f);
      }

      files::free_image(*img);
    }
  }
}

static void load_char_sheet_quads(surge::atom::sprite::data_list &dl, float ww, float wh) noexcept {
  using namespace surge;

  for (usize i = 0; i < char_sheet_img_count; i++) {
    dl.models.push_back(glm::scale(glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, 0.0f}),
                                   glm::vec3{ww, wh, 1.0}));
  }
}

auto DTU::state::new_game::load(surge::deque<surge::u32> &, surge::atom::sprite::data_list &dl,
                                float ww, float wh) noexcept -> int {
  using namespace surge;

  log_info("Loading new_game state");

  // Character Sheet
  load_char_sheet_imgs(dl);
  load_char_sheet_quads(dl, ww, wh);

  atom::sprite::make_resident(dl.texture_handles);

  // First command

  return 0;
}

void DTU::state::new_game::unload(surge::deque<surge::u32> &cmdq,
                                  surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Unloading main_menu state");

  atom::sprite::make_non_resident(dl.texture_handles);
  atom::sprite::destroy_texture(dl.texture_ids);
  dl.texture_handles.clear();
  dl.texture_ids.clear();
  dl.alphas.clear();
  dl.models.clear();

  cmdq.clear();

  return;
}

void DTU::state::new_game::update(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &,
                                  double) noexcept {

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {
  default:
    break;
  }
}

void DTU::state::new_game::keyboard_event(surge::deque<surge::u32> &, int, int, int, int) noexcept {
  // TODO
  return;
}