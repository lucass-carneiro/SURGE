#include "compute.hpp"

#include "sc_logging.hpp"
#include "sc_vulkan/atoms/descriptor.hpp"

#include <array>

namespace vk_globals {

static surge::vk_atom::descriptor::desc_alloc desc_alloc{};
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
  using namespace surge::vk_atom::descriptor;

  // create a descriptor pool that will hold 10 sets with 1 image each
  log_info("Creating descriptor pool");
  std::array<desc_pool_size_ratio, 1> sizes{{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  vk_globals::desc_alloc.init_pool(ctx, 10, sizes);

  // Make the descriptor set layout for the compute shader
  log_info("Allocating descriptor set");

  desc_builder builder{};
  builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  const auto layout{builder.build(ctx, VK_SHADER_STAGE_COMPUTE_BIT)};

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
  vk_globals::desc_alloc.clear_descriptors(ctx);
  destroy_desc_layout(ctx, vk_globals::img_desc_set_layout);

  log_info("Destroying descriptor pool");
  vk_globals::desc_alloc.destroy_pool(ctx);

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

  VkWriteDescriptorSet draw_img_ds_write = {};
  draw_img_ds_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  draw_img_ds_write.pNext = nullptr;

  draw_img_ds_write.dstBinding = 0;
  draw_img_ds_write.dstSet = vk_globals::img_desc_set;
  draw_img_ds_write.descriptorCount = 1;
  draw_img_ds_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  draw_img_ds_write.pImageInfo = &draw_img_info;

  update_desc_sets(ctx, 1, &draw_img_ds_write, 0, nullptr);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void vk_keyboard_event(surge::window::window_t, int, int, int, int) {
}

extern "C" SURGE_MODULE_EXPORT void vk_mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void vk_mouse_scroll_event(surge::window::window_t, double, double) {
}
