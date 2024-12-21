#include "sc_vulkan_desc_alloc.hpp"

#include "sc_container_types.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::desc_alloc::init_pool(VkDevice device, u32 max_sets,
                                                std::span<desc_pool_size_ratio> pool_ratios)
    -> tl::expected<void, error> {
  vector<VkDescriptorPoolSize> pool_sizes{};
  for (const auto &ratio : pool_ratios) {
    pool_sizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type, .descriptorCount = static_cast<u32>(ratio.ratio * max_sets)});
  }

  VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pool_info.flags = 0;
  pool_info.maxSets = max_sets;
  pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  const auto result{vkCreateDescriptorPool(device, &pool_info, get_alloc_callbacks(), &pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to create descriptor pool: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_pool_init};
  }
}

auto surge::renderer::vk::desc_alloc::clear_descriptors(VkDevice device)
    -> tl::expected<void, error> {
  const auto result{vkResetDescriptorPool(device, pool, 0)};
  if (result != VK_SUCCESS) {
    log_error("Unable to reset descriptor pool: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_pool_reset};
  }
}

void surge::renderer::vk::desc_alloc::destroy_pool(VkDevice device) {
  vkDestroyDescriptorPool(device, pool, get_alloc_callbacks());
}

auto surge::renderer::vk::desc_alloc::allocate(VkDevice device, VkDescriptorSetLayout layout)
    -> tl::expected<VkDescriptorSet, error> {
  VkDescriptorSetAllocateInfo ai = {};
  ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  ai.pNext = nullptr;
  ai.descriptorPool = pool;
  ai.descriptorSetCount = 1;
  ai.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  const auto result{vkAllocateDescriptorSets(device, &ai, &ds)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate descriptor sets: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_set_alloc};
  }

  return ds;
}