#ifndef SURGE_CORE_RENDERER_VK_IMAGES_HPP
#define SURGE_CORE_RENDERER_VK_IMAGES_HPP

#include "config.hpp"
#include "renderer_vk_types.hpp"

#include <tl/expected.hpp>

namespace surge::renderer::vk {

auto imageview_create_info(VkFormat format, VkImage image,
                           VkImageAspectFlags aspect_flags) noexcept -> VkImageViewCreateInfo;

auto image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
                       VkExtent3D extent) noexcept -> VkImageCreateInfo;

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout curr_layout,
                      VkImageLayout new_layout) noexcept;
void image_blit(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D src_size,
                VkExtent2D dst_size) noexcept;
auto image_subresource_range(VkImageAspectFlags aspect_mask) noexcept -> VkImageSubresourceRange;

auto create_draw_img(const config::window_resolution &w_res, VkDevice logi_dev,
                     VmaAllocator allocator) noexcept -> tl::expected<allocated_image, error>;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_IMAGES_HPP