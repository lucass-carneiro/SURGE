#include "sc_allocators.hpp"
#include "sc_logging.hpp"
#include "sc_options.hpp"
#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_vulkan_debug.hpp"
#include "sc_vulkan_images.hpp"
#include "sc_vulkan_init.hpp"
#include "sc_vulkan_malloc.hpp"
#include "sc_vulkan_types.hpp"

namespace surge::renderer::vk {

auto initialize(window::window_t w, const config::renderer_attrs &r_attrs,
                const config::window_resolution &w_res,
                const config::window_attrs &) -> tl::expected<context, error> {

  log_info("Initializing Vulkan");

  // Alloc context
  auto ctx = static_cast<context>(allocators::mimalloc::malloc(sizeof(context_t)));
  if (!ctx) {
    log_error("Unable to allocate memory for Vulkan context");
    return tl::unexpected{error::vk_ctx_alloc};
  }

  new (ctx)(context_t)();

  // API version
  const auto api_version{get_api_version()};
  if (!api_version) {
    return tl::unexpected{api_version.error()};
  }

  // Extensions
  const auto instance_extensions{get_required_extensions()};
  if (!instance_extensions) {
    return tl::unexpected{instance_extensions.error()};
  }

  // Validation layers
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  const auto validation_layers{get_required_validation_layers()};
  if (!validation_layers) {
    return tl::unexpected{validation_layers.error()};
  }

  auto dbg_msg_ci{dbg_msg_create_info()};
#endif

// Instance
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  const auto instance{build_instance(*instance_extensions, *validation_layers, dbg_msg_ci)};
  if (!instance) {
    return tl::unexpected{instance.error()};
  } else {
    ctx->instance = *instance;
  }
#else
  const auto instance{build_instance(*instance_extensions)};
  if (!instance) {
    return tl::unexpected{instance.error()};
  } else {
    ctx->instance = *instance;
  }
#endif

// Debug MSG
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  const auto dbg_msg{create_dbg_msg(*instance, dbg_msg_ci)};
  if (!dbg_msg) {
    return tl::unexpected{dbg_msg.error()};
  } else {
    ctx->dbg_msg = *dbg_msg;
  }
#endif

  // Physical device selection
  const auto phys_dev{select_physical_device(*instance)};
  if (!phys_dev) {
    return tl::unexpected{phys_dev.error()};
  } else {
    ctx->phys_dev = *phys_dev;
  }

  // Logical device creation
  const auto device{create_logical_device(*phys_dev)};
  if (!device) {
    return tl::unexpected{device.error()};
  } else {
    ctx->device = *device;
  }

  // Window surface
  const auto surface{create_window_surface(w, *instance)};
  if (!surface) {
    return tl::unexpected{surface.error()};
  } else {
    ctx->surface = *surface;
  }

  const auto q_handles{get_queue_handles(*phys_dev, *device, *surface)};
  if (!q_handles) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx->q_handles = *q_handles;
  }

  const auto swpc_data{create_swapchain(*phys_dev, *device, *surface, r_attrs,
                                        static_cast<u32>(w_res.width),
                                        static_cast<u32>(w_res.height))};
  if (!swpc_data) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx->swpc_data = *swpc_data;
  }

  const auto frm_data{create_frame_data(*device, q_handles->graphics_idx)};
  if (!frm_data) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx->frm_data = *frm_data;
  }

  const auto allocator{create_memory_allocator(*instance, *phys_dev, *device)};
  if (!allocator) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx->allocator = *allocator;
  }

  const auto draw_image{create_draw_img(w_res, *device, *allocator)};
  if (!draw_image) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx->draw_image = *draw_image;
  }

  // Initialization done
  log_info("Initialized Vulkan context, handle {}", static_cast<void *>(ctx));

  return ctx;
}

void terminate(context ctx) {
  log_info("Destroying Vulkan context handle {}", static_cast<void *>(ctx));

  log_info("Terminating Vulkan");
  const auto alloc_callbacks{get_alloc_callbacks()};

  log_info("Waiting for GPU idle");
  vkWaitForFences(ctx->device, ctx->frm_data.frame_overlap, ctx->frm_data.render_fences.data(),
                  true, 1000000000);

  log_info("Destroying draw image");
  vkDestroyImageView(ctx->device, ctx->draw_image.image_view, get_alloc_callbacks());
  vmaDestroyImage(ctx->allocator, ctx->draw_image.image, ctx->draw_image.allocation);

  log_info("Destroying memory allocator");
  vmaDestroyAllocator(ctx->allocator);

  destroy_frame_data(ctx->device, ctx->frm_data);

  log_info("Destroying image views");
  for (const auto &img_view : ctx->swpc_data.imgs_views) {
    vkDestroyImageView(ctx->device, img_view, alloc_callbacks);
  }

  log_info("Destroying swapchain");
  vkDestroySwapchainKHR(ctx->device, ctx->swpc_data.swapchain, alloc_callbacks);

  log_info("Destroying window surface");
  vkDestroySurfaceKHR(ctx->instance, ctx->surface, alloc_callbacks);

  log_info("Destroying logical device");
  vkDestroyDevice(ctx->device, alloc_callbacks);

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  destroy_dbg_msg(ctx->instance, ctx->dbg_msg);
#endif

  log_info("Terminating instnace");
  vkDestroyInstance(ctx->instance, alloc_callbacks);

  // Free context
  ctx->~context_t();
  allocators::mimalloc::free(ctx);

  log_info("Vulkan context {} terminated", static_cast<void *>(ctx));
}

} // namespace surge::renderer::vk