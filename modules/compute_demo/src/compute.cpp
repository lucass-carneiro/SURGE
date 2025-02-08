#include "compute.hpp"

#include "sc_logging.hpp"
#include "sc_vulkan/atoms/descriptor.hpp"

#include <array>

namespace vk_globals {

static surge::vk_atom::descriptor::allocator desc_alloc{};
static surge::vk_atom::descriptor::writer desc_writer{};
static VkDescriptorSet img_desc_set{};
static VkDescriptorSetLayout img_desc_set_layout{};

} // namespace vk_globals

extern "C" SURGE_MODULE_EXPORT auto gl_on_load(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto gl_on_unload(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto gl_draw(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto gl_update(surge::window::window_t, double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void gl_keyboard_event(surge::window::window_t, int, int, int, int) {
}

extern "C" SURGE_MODULE_EXPORT void gl_mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void gl_mouse_scroll_event(surge::window::window_t, double, double) {
}

extern "C" SURGE_MODULE_EXPORT auto vk_on_load(surge::window::window_t,
                                               surge::renderer::vk::context ctx) -> int {
  using namespace surge;
  using namespace surge::vk_atom;

  log_info("Creating descriptor allocator");
  std::array<descriptor::pool_size_ratio, 1> ratios{
      descriptor::pool_size_ratio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  vk_globals::desc_alloc.init(ctx, 10, ratios);

  // Make the descriptor set layout for the compute shader
  log_info("Allocating descriptor set");

  descriptor::builder desc_builder{};
  desc_builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  const auto layout{desc_builder.build(ctx, VK_SHADER_STAGE_COMPUTE_BIT)};

  if (!layout) {
    log_error("Unable to create descriptor layout for the compute shader's image");
    return static_cast<int>(layout.error());
  } else {
    vk_globals::img_desc_set_layout = *layout;
  }

  // Allocate the descriptor set
  const auto desc{vk_globals::desc_alloc.allocate(ctx, vk_globals::img_desc_set_layout)};
  if (!desc) {
    log_error("Unable to allocate descriptor set for compute shader");
    return static_cast<int>(desc.error());
  } else {
    vk_globals::img_desc_set = *desc;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto vk_on_unload(surge::window::window_t,
                                                 surge::renderer::vk::context ctx) -> int {
  using namespace surge;
  using namespace surge::vk_atom::descriptor;

  log_info("Destroying descriptors");
  vk_globals::desc_alloc.clear_pools(ctx);
  destroy_desc_layout(ctx, vk_globals::img_desc_set_layout);

  log_info("Destroying descriptor pool");
  vk_globals::desc_alloc.destroy_pools(ctx);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto vk_draw(surge::window::window_t, surge::renderer::vk::context)
    -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto vk_update(surge::window::window_t,
                                              surge::renderer::vk::context ctx, double) -> int {
  using namespace surge;
  using namespace surge::vk_atom::descriptor;

  // Update descriptor sets
  auto draw_img_info{draw_img_desc_info(ctx)};

  vk_globals::desc_writer.write_image(0, draw_img_info.imageView, VK_NULL_HANDLE,
                                      VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  vk_globals::desc_writer.update_set(ctx, vk_globals::img_desc_set);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void vk_keyboard_event(surge::window::window_t, int, int, int, int) {
}

extern "C" SURGE_MODULE_EXPORT void vk_mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void vk_mouse_scroll_event(surge::window::window_t, double, double) {
}
