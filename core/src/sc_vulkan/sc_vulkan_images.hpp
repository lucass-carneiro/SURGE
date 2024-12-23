#ifndef SURGE_CORE_RENDERER_VULKAN_IMAGES_HPP
#define SURGE_CORE_RENDERER_VULKAN_IMAGES_HPP

#include "sc_config.hpp"
#include "sc_vulkan_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

#include <tl/expected.hpp>

namespace surge::renderer::vk {

auto imageview_create_info(VkFormat format, VkImage image,
                           VkImageAspectFlags aspect_flags) -> VkImageViewCreateInfo;

auto image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
                       VkExtent3D extent) -> VkImageCreateInfo;

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout curr_layout,
                      VkImageLayout new_layout);
void image_blit(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D src_size,
                VkExtent2D dst_size);
auto image_subresource_range(VkImageAspectFlags aspect_mask) -> VkImageSubresourceRange;

auto create_draw_img(const config::window_resolution &w_res, VkDevice logi_dev,
                     VmaAllocator allocator) -> tl::expected<allocated_image, error>;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_IMAGES_HPP