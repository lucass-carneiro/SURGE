#include "sc_vulkan_images.hpp"

#include "sc_integer_types.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::imageview_create_info(VkFormat format, VkImage image,
                                                VkImageAspectFlags aspect_flags) noexcept
    -> VkImageViewCreateInfo {
  VkImageViewCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ci.pNext = nullptr;

  ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ci.image = image;
  ci.format = format;
  ci.subresourceRange.baseMipLevel = 0;
  ci.subresourceRange.levelCount = 1;
  ci.subresourceRange.baseArrayLayer = 0;
  ci.subresourceRange.layerCount = 1;
  ci.subresourceRange.aspectMask = aspect_flags;
  return ci;
}

auto surge::renderer::vk::image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
                                            VkExtent3D extent) noexcept -> VkImageCreateInfo {
  VkImageCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ci.pNext = nullptr;

  ci.imageType = VK_IMAGE_TYPE_2D;

  ci.format = format;
  ci.extent = extent;

  ci.mipLevels = 1;
  ci.arrayLayers = 1;

  // TODO: Enable MSAA when requested.
  ci.samples = VK_SAMPLE_COUNT_1_BIT;

  ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  ci.usage = usage_flags;
  return ci;
}

void surge::renderer::vk::transition_image(VkCommandBuffer cmd, VkImage image,
                                           VkImageLayout curr_layout,
                                           VkImageLayout new_layout) noexcept {
  // See https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples for finer sync
  // options

  VkImageAspectFlags aspect_mask((new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                     ? VK_IMAGE_ASPECT_DEPTH_BIT
                                     : VK_IMAGE_ASPECT_COLOR_BIT);

  VkImageMemoryBarrier2 img_barrier{};
  img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  img_barrier.pNext = nullptr;

  img_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  img_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  img_barrier.oldLayout = curr_layout;
  img_barrier.newLayout = new_layout;

  img_barrier.image = image;
  img_barrier.subresourceRange = image_subresource_range(aspect_mask);

  VkDependencyInfo dep_info{};
  dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep_info.pNext = nullptr;

  dep_info.imageMemoryBarrierCount = 1;
  dep_info.pImageMemoryBarriers = &img_barrier;

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

void surge::renderer::vk::image_blit(VkCommandBuffer cmd, VkImage source, VkImage destination,
                                     VkExtent2D src_size, VkExtent2D dst_size) noexcept {
  VkImageBlit2 blit_reg{};
  blit_reg.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
  blit_reg.pNext = nullptr;

  blit_reg.srcOffsets[1].x = static_cast<i32>(src_size.width);
  blit_reg.srcOffsets[1].y = static_cast<i32>(src_size.height);
  blit_reg.srcOffsets[1].z = 1;

  blit_reg.dstOffsets[1].x = static_cast<i32>(dst_size.width);
  blit_reg.dstOffsets[1].y = static_cast<i32>(dst_size.height);
  blit_reg.dstOffsets[1].z = 1;

  blit_reg.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_reg.srcSubresource.baseArrayLayer = 0;
  blit_reg.srcSubresource.layerCount = 1;
  blit_reg.srcSubresource.mipLevel = 0;

  blit_reg.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_reg.dstSubresource.baseArrayLayer = 0;
  blit_reg.dstSubresource.layerCount = 1;
  blit_reg.dstSubresource.mipLevel = 0;

  VkBlitImageInfo2 blitInfo{};
  blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blitInfo.pNext = nullptr;
  blitInfo.dstImage = destination;
  blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.srcImage = source;
  blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.filter = VK_FILTER_LINEAR;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &blit_reg;

  vkCmdBlitImage2(cmd, &blitInfo);
}

auto surge::renderer::vk::image_subresource_range(VkImageAspectFlags aspect_mask) noexcept
    -> VkImageSubresourceRange {
  VkImageSubresourceRange sum_img{};
  sum_img.aspectMask = aspect_mask;
  sum_img.baseMipLevel = 0;
  sum_img.levelCount = VK_REMAINING_MIP_LEVELS;
  sum_img.baseArrayLayer = 0;
  sum_img.layerCount = VK_REMAINING_ARRAY_LAYERS;
  return sum_img;
}

auto surge::renderer::vk::create_draw_img(const config::window_resolution &w_res, VkDevice logi_dev,
                                          VmaAllocator allocator) noexcept
    -> tl::expected<allocated_image, error> {
  log_info("Creating draw image target");

  allocated_image draw_image{};

  VkExtent3D draw_image_extent{static_cast<u32>(w_res.width), static_cast<u32>(w_res.height), 1};
  draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
  draw_image.image_extent = draw_image_extent;

  VkImageUsageFlags draw_image_usage_flags{};
  draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto rimg_info{
      image_create_info(draw_image.image_format, draw_image_usage_flags, draw_image_extent)};

  VmaAllocationCreateInfo rimg_allocinfo{};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // Allocate and create the image
  auto result{vmaCreateImage(allocator, &rimg_info, &rimg_allocinfo, &draw_image.image,
                             &draw_image.allocation, nullptr)};
  if (result != VK_SUCCESS) {
    log_error("Unable to create draw image: {}", string_VkResult(result));
    return tl::unexpected{error::vk_init_draw_img};
  }

  // Build a image-view for the draw image to use for rendering
  auto rview_info{
      imageview_create_info(draw_image.image_format, draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT)};

  result = vkCreateImageView(logi_dev, &rview_info, get_alloc_callbacks(), &draw_image.image_view);

  if (result != VK_SUCCESS) {
    log_error("Unable to create draw image view: {}", string_VkResult(result));
    return tl::unexpected{error::vk_init_draw_img};
  }

  log_info("Draw image created");
  return draw_image;
}