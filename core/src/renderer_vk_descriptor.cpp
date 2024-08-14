#include "renderer_vk_descriptor.hpp"

#include "logging.hpp"
#include "renderer_vk_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

using namespace surge::renderer::vk;

void descriptor_layout::add_binding(u32 binding, VkDescriptorType type) noexcept {
  VkDescriptorSetLayoutBinding lb{};
  lb.binding = binding;
  lb.descriptorCount = 1;
  lb.descriptorType = type;

  bindings.push_back(lb);
}

void descriptor_layout::clear() noexcept { bindings.clear(); }

auto descriptor_layout::build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext,
                              VkDescriptorSetLayoutCreateFlags flags) noexcept
    -> tl::expected<VkDescriptorSetLayout, error> {

  for (auto &b : bindings) {
    b.stageFlags |= shaderStages;
  }

  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext = pNext;

  info.pBindings = bindings.data();
  info.bindingCount = static_cast<u32>(bindings.size());
  info.flags = flags;

  VkDescriptorSetLayout set{};
  const auto result{vkCreateDescriptorSetLayout(device, &info, get_alloc_callbacks(), &set)};

  if (result != VK_SUCCESS) {
    log_error("Unable to build descriptor set layout: {}", string_VkResult(result));
    return tl::unexpected{error::vk_descriptor_set_layout};
  }

  return set;
}

auto surge::renderer::vk::create_pool(VkDevice device, u32 maxSets,
                                      std::span<descriptor_pool_size_ratio> poolRatios) noexcept
    -> tl::expected<VkDescriptorPool, error> {

  vector<VkDescriptorPoolSize> poolSizes{};

  for (const auto &ratio : poolRatios) {
    VkDescriptorPoolSize ps{};
    ps.type = ratio.type;
    ps.descriptorCount = static_cast<u32>(ratio.ratio * static_cast<float>(maxSets));
    poolSizes.push_back(ps);
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = maxSets;
  pool_info.poolSizeCount = static_cast<u32>(poolSizes.size());
  pool_info.pPoolSizes = poolSizes.data();

  VkDescriptorPool pool{};
  const auto result{vkCreateDescriptorPool(device, &pool_info, get_alloc_callbacks(), &pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to build descriptor pool: {}", string_VkResult(result));
    return tl::unexpected{error::vk_descriptor_pool};
  }

  return pool;
}

void surge::renderer::vk::reset_pool(VkDevice device, VkDescriptorPool pool) noexcept {
  vkResetDescriptorPool(device, pool, 0);
}

void surge::renderer::vk::destroy_pool(VkDevice device, VkDescriptorPool pool) noexcept {
  vkDestroyDescriptorPool(device, pool, get_alloc_callbacks());
}

auto surge::renderer::vk::alloc_from_pool(VkDevice device, VkDescriptorPool pool,
                                          VkDescriptorSetLayout layout) noexcept
    -> tl::expected<VkDescriptorSet, error> {

  VkDescriptorSetAllocateInfo info;
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.pNext = nullptr;
  info.descriptorPool = pool;
  info.descriptorSetCount = 1;
  info.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  const auto result{vkAllocateDescriptorSets(device, &info, &ds)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate descriptor sets: {}", string_VkResult(result));
    return tl::unexpected{error::vk_descriptor_set_alloc};
  }

  return ds;
}
