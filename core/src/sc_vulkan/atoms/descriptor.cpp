#include "sc_vulkan/atoms/descriptor.hpp"

#include "../sc_vulkan_malloc.hpp"
#include "../sc_vulkan_types.hpp"
#include "sc_container_types.hpp"
#include "sc_logging.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::vk_atom::descriptor::desc_alloc::init_pool(renderer::vk::context ctx, u32 max_sets,
                                                       std::span<desc_pool_size_ratio> pool_ratios)
    -> tl::expected<void, error> {
  vector<VkDescriptorPoolSize> pool_sizes{};
  for (const auto &ratio : pool_ratios) {
    pool_sizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type, .descriptorCount = static_cast<u32>(ratio.ratio * max_sets)});
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.pNext = nullptr;
  pool_info.maxSets = max_sets;
  pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  const auto result{
      vkCreateDescriptorPool(ctx->device, &pool_info, renderer::vk::get_alloc_callbacks(), &pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to create descriptor pool: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_pool_init};
  }

  return {};
}

auto surge::vk_atom::descriptor::desc_alloc::clear_descriptors(renderer::vk::context ctx)
    -> tl::expected<void, error> {
  const auto result{vkResetDescriptorPool(ctx->device, pool, 0)};
  if (result != VK_SUCCESS) {
    log_error("Unable to reset descriptor pool: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_pool_reset};
  }

  return {};
}

void surge::vk_atom::descriptor::desc_alloc::destroy_pool(renderer::vk::context ctx) {
  vkDestroyDescriptorPool(ctx->device, pool, renderer::vk::get_alloc_callbacks());
}

auto surge::vk_atom::descriptor::desc_alloc::allocate(renderer::vk::context ctx,
                                                      VkDescriptorSetLayout layout)
    -> tl::expected<VkDescriptorSet, error> {
  VkDescriptorSetAllocateInfo ai = {};
  ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  ai.pNext = nullptr;
  ai.descriptorPool = pool;
  ai.descriptorSetCount = 1;
  ai.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  const auto result{vkAllocateDescriptorSets(ctx->device, &ai, &ds)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate descriptor sets: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_set_alloc};
  }

  return ds;
}

auto surge::vk_atom::descriptor::draw_img_desc_info(renderer::vk::context ctx)
    -> VkDescriptorImageInfo {
  VkDescriptorImageInfo info{};
  info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  info.imageView = ctx->draw_image.image_view;

  return info;
}

void surge::vk_atom::descriptor::desc_builder::add_binding(u32 binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding b{};
  b.binding = binding;
  b.descriptorCount = 1;
  b.descriptorType = type;

  bindings.push_back(b);
}

void surge::vk_atom::descriptor::desc_builder::clear() { bindings.clear(); }

auto surge::vk_atom::descriptor::desc_builder::build(
    renderer::vk::context ctx, VkShaderStageFlags shader_stages, void *pNext,
    VkDescriptorSetLayoutCreateFlags flags) -> tl::expected<VkDescriptorSetLayout, error> {
  for (auto &b : bindings) {
    b.stageFlags |= shader_stages;
  }

  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext = pNext;
  info.pBindings = bindings.data();
  info.bindingCount = static_cast<u32>(bindings.size());
  info.flags = flags;

  VkDescriptorSetLayout set{};
  const auto result{
      vkCreateDescriptorSetLayout(ctx->device, &info, renderer::vk::get_alloc_callbacks(), &set)};

  if (result != VK_SUCCESS) {
    log_error("Unable to create descriptor set layout: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_set_layout_build};
  }

  return set;
}

void surge::vk_atom::descriptor::destroy_desc_layout(renderer::vk::context ctx,
                                                     VkDescriptorSetLayout layout) {
  vkDestroyDescriptorSetLayout(ctx->device, layout, renderer::vk::get_alloc_callbacks());
}

void surge::vk_atom::descriptor::update_desc_sets(renderer::vk::context ctx, u32 write_count,
                                                  const VkWriteDescriptorSet *desc_writes,
                                                  u32 copy_count,
                                                  const VkCopyDescriptorSet *desc_copies) {
  vkUpdateDescriptorSets(ctx->device, write_count, desc_writes, copy_count, desc_copies);
}