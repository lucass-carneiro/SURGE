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

  // Create Sprite database objct
  sprite_database::CreateInfo sdb_ci{.blending_mode = sprite_database::BlendingMode::alpha,
                                     .max_sprites = 3};

  const auto sdb{sprite_database::create(mod_ctx->vk_ctx, sdb_ci, globals::desc_alloc)};
  if (!sdb) {
    return sdb.error();
  }
  globals::sdb = *sdb;

  // Load sprite textures
  std::array<const char *, 3> paths{"img_1.png", "img_2.png", "img_3.png"};
  sprite_database::upload_images(mod_ctx->vk_ctx, globals::sdb, paths);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge::renderer::vk::atom;
  sprite_database::destroy(mod_ctx->vk_ctx, globals::sdb);
  globals::desc_alloc.destroy_pools(mod_ctx->vk_ctx);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge::renderer::vk::atom;
  sprite_database::draw(mod_ctx->vk_ctx, globals::sdb, globals::projection_matrix,
                        globals::view_matrix);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(surge::module::Context mod_ctx, double) noexcept -> int {
  using namespace surge::renderer::vk::atom;
  using namespace surge::window;
  using surge::usize;

  // Geometry constants
  const auto window_dims{get_dims(mod_ctx->window)};
  const glm::vec2 scale{100.0f};
  const float z{0.5f};

  const auto dx{1.0f};
  const auto dy{-1.0f};

  // Sprite positions
  static std::array<glm::vec2, 3> positions{
      glm::vec2{10.0f},
      glm::vec2{200.0f},
      glm::vec2{400.0f},
  };

  // Add sprites
  for (usize i = 0; auto &pos : positions) {
    // pos[0] += dx;
    // pos[1] += dy;
    //
    // if (pos[0] > window_dims[0]) {
    //   pos[0] = -scale[0];
    // }
    //
    // if ((pos[0] + scale[0]) < 0.0f) {
    //   pos[0] = window_dims[0];
    // }
    //
    // if (pos[1] > window_dims[1]) {
    //   pos[1] = -scale[1];
    // }
    //
    // if ((pos[1] + scale[1]) < 0.0f) {
    //   pos[1] = window_dims[1];
    // }

    const sprite_database::UpdateInfo update_info{
        .position = pos, .scale = scale, .z = z, .texture_id = i};

    sprite_database::update_draw_data(globals::sdb, update_info);

    i++;
  }

  // Send them to the GPU
  sprite_database::synchronize_draw_data(mod_ctx->vk_ctx, globals::sdb);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window, int, int, int,
                                                   int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window, int, int,
                                                       int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window, double,
                                                       double) noexcept {}
