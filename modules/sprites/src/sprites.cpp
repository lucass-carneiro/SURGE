#include "sprites.hpp"

#include "sc_vulkan/atoms/sprite_database.hpp"
#include "sc_window.hpp"

using surge::renderer::vk::atom::sprite_database::SpriteDatabase;

namespace globals {
static glm::mat4 projection_matrix{1.0};
static glm::mat4 view_matrix{1.0};
static SpriteDatabase sdb{nullptr};
} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge;
  using namespace surge::renderer::vk::atom;

  // Projection and view matrices
  const auto dims{window::get_dims(mod_ctx->window)};
  globals::projection_matrix = sprite_database::make_ortho_projection(dims);
  globals::view_matrix = sprite_database::make_view(glm::vec2{0.0f});

  // Sprite database
  constexpr usize max_sprites{3};
  const auto sdb{sprite_database::create(mod_ctx->vk_ctx, max_sprites)};
  if (!sdb) {
    return sdb.error();
  }
  globals::sdb = *sdb;

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge::renderer::vk::atom;
  sprite_database::destroy(globals::sdb, mod_ctx->vk_ctx);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge::renderer::vk::atom;
  sprite_database::draw(globals::sdb, mod_ctx->vk_ctx, globals::projection_matrix,
                        globals::view_matrix);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(surge::module::Context mod_ctx, double) noexcept -> int {
  using namespace surge::renderer::vk::atom;

  // Add sprites
  sprite_database::push(globals::sdb, glm::vec2{0.0f}, glm::vec2{50.0f}, 1.0f);
  sprite_database::push(globals::sdb, glm::vec2{100.0f}, glm::vec2{50.0f}, 1.0f);
  sprite_database::push(globals::sdb, glm::vec2{200.0f}, glm::vec2{50.0f}, 1.0f);

  // Send them to the GPU
  sprite_database::sync(globals::sdb, mod_ctx->vk_ctx);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window, int, int, int,
                                                   int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window, int, int,
                                                       int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window, double,
                                                       double) noexcept {}
