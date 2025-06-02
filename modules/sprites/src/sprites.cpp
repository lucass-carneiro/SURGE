#include "sprites.hpp"

#include "sc_vulkan/atoms/sprite_database.hpp"
#include "sc_vulkan/sc_vulkan_descriptor.hpp"
#include "sc_window.hpp"

using surge::renderer::vk::DescriptorPoolSizeRatio;
using surge::renderer::vk::GrowableDescriptorAllocator;
using surge::renderer::vk::atom::sprite_database::SpriteDatabase;

namespace globals {
static glm::mat4 projection_matrix{1.0};
static glm::mat4 view_matrix{1.0};
static GrowableDescriptorAllocator desc_alloc{};
static SpriteDatabase sdb{nullptr};
} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge;
  using namespace surge::renderer::vk::atom;

  // Projection and view matrices
  const auto dims{window::get_dims(mod_ctx->window)};
  globals::projection_matrix = sprite_database::make_ortho_projection(dims);
  globals::view_matrix = sprite_database::make_view(glm::vec2{0.0f});

  // Descriptor allocator
  std::array<DescriptorPoolSizeRatio, 1> pool_ratios{
      DescriptorPoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f}};
  globals::desc_alloc.init(mod_ctx->vk_ctx, 8, pool_ratios);

  // Sprite database
  sprite_database::SpriteDatabaseCreateInfo sdb_ci{
      .ctx = mod_ctx->vk_ctx,
      .blending_mode = sprite_database::SpriteDatabaseBlendingMode::alpha,
      .max_sprites = 3};

  const auto sdb{sprite_database::create(sdb_ci, globals::desc_alloc)};
  if (!sdb) {
    return sdb.error();
  }
  globals::sdb = *sdb;

  sprite_database::SpriteDatabaseAddInfo ai{.ctx = mod_ctx->vk_ctx,
                                            .texture_path = nullptr,
                                            .position = glm::vec2{0.0f},
                                            .scale = glm::vec2{100.0f},
                                            .z = 1.0f,
                                            .color_multiplier = glm::vec4{1.0f}};
  sprite_database::add(globals::sdb, ai);

  ai.position = glm::vec2{200.0f};
  sprite_database::add(globals::sdb, ai);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge::renderer::vk::atom;
  sprite_database::destroy(globals::sdb, mod_ctx->vk_ctx);
  globals::desc_alloc.destroy_pools(mod_ctx->vk_ctx);
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
  sprite_database::SpriteDatabasePushInfo pi{.position = glm::vec2{0.0f},
                                             .scale = glm::vec2{100.0f},
                                             .z = 1.0f,
                                             .color_multiplier = glm::vec4{1.0f}};

  sprite_database::push(globals::sdb, pi);

  pi.position = glm::vec2{200.0f};
  sprite_database::push(globals::sdb, pi);

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
