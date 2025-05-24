#include "sprites.hpp"

#include "sc_vulkan/atoms/sprite_database.hpp"
#include "sc_window.hpp"

using surge::renderer::vk::atom::sprite_database::SpriteDatabase;
using surge::renderer::vk::atom::sprite_database::WorldMatrices;

namespace globals {
static WorldMatrices wmat{};
static SpriteDatabase sdb{nullptr};
} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge;
  using namespace surge::renderer::vk::atom;

  // Projection and view matrices
  const auto dims{window::get_dims(mod_ctx->window)};
  globals::wmat.projection = sprite_database::make_ortho_projection(dims);
  globals::wmat.view = sprite_database::make_view(glm::vec2{0.0f});

  globals::wmat.model
      = sprite_database::make_model_matrix(glm::vec2{0.0f}, glm::vec2{100.0f}, 1.0f);

  // Sprite database
  const auto sdb{sprite_database::create(mod_ctx->vk_ctx)};
  if (!sdb) {
    return static_cast<int>(sdb.error());
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
  sprite_database::draw(globals::sdb, mod_ctx->vk_ctx, globals::wmat);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(surge::module::Context, double) noexcept -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window, int, int, int,
                                                   int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window, int, int,
                                                       int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window, double,
                                                       double) noexcept {}
