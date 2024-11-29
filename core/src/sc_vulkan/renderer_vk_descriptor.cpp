#include "renderer_vk_descriptor.hpp"

#include "logging.hpp"
#include "renderer_vk_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

using namespace surge::renderer::vk;

auto surge::renderer::vk::descriptor_layout_builder::build(
    VkDevice device, VkShaderStageFlags shaderStages, void *pNext,
    VkDescriptorSetLayoutCreateFlags flags) noexcept -> tl::expected<VkDescriptorSetLayout, error> {

  for (auto &b : bindings) {
    b.stageFlags |= shaderStages;
  }

  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext = pNext;

  info.pBindings = bindings.data();
  info.bindingCount = (uint32_t)bindings.size();
  info.flags = flags;

  VkDescriptorSetLayout set{};
  const auto result{vkCreateDescriptorSetLayout(device, &info, nullptr, &set)};

  if (result != VK_SUCCESS) {
    log_error("Unable to build descriptor set layout: {}", string_VkResult(result));
    return tl::unexpected{error::vk_descriptor_set_layout};
  }

  return set;
}

void descriptor_layout_builder::add_binding(u32 binding, VkDescriptorType type) noexcept {
  VkDescriptorSetLayoutBinding lb{};
  lb.binding = binding;
  lb.descriptorCount = 1;
  lb.descriptorType = type;

  bindings.push_back(lb);
}

void descriptor_layout_builder::clear() noexcept { bindings.clear(); }

auto surge::renderer::vk::descriptor_allocator::create_pool(
    VkDevice device, uint32_t set_count,
    std::span<desc_pool_size_ratio> pool_ratios) noexcept -> tl::expected<VkDescriptorPool, error> {

  vector<VkDescriptorPoolSize> pool_sizes{};

  for (const auto &ratio : pool_ratios) {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = ratio.type;
    pool_size.descriptorCount = static_cast<u32>(ratio.ratio * static_cast<float>(set_count));

    pool_sizes.push_back(pool_size);
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.pNext = nullptr;
  pool_info.flags = 0;
  pool_info.maxSets = set_count;
  pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  VkDescriptorPool pool{};
  const auto result{vkCreateDescriptorPool(device, &pool_info, get_alloc_callbacks(), &pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate descriptor pool: {}", string_VkResult(result));
    return tl::unexpected{error::vk_descriptor_pool};
  }

  return pool;
}

auto surge::renderer::vk::descriptor_allocator::get_pool(VkDevice device) noexcept
    -> tl::expected<VkDescriptorPool, error> {

  VkDescriptorPool pool{};

  if (ready_pools.size() != 0) {
    pool = ready_pools.back();
    ready_pools.pop_back();
  } else {
    const auto pool_result{create_pool(device, sets_per_pool, ratios)};

    if (!pool_result) {
      log_error("Unable to create new descriptor pool for expanding the descriptor allocator");
      return tl::unexpected{pool_result.error()};
    }

    sets_per_pool += sets_per_pool / 2;
    if (sets_per_pool > 4092) {
      sets_per_pool = 4092;
    }
  }

  return pool;
}

auto surge::renderer::vk::descriptor_allocator::init(
    VkDevice device, uint32_t initial_sets, std::span<desc_pool_size_ratio> pool_ratios) noexcept
    -> tl::expected<descriptor_allocator, error> {
  log_info("Initializing descriptor allocator");

  descriptor_allocator alloc{};

  alloc.ratios.clear();

  for (const auto &ratio : pool_ratios) {
    alloc.ratios.push_back(ratio);
  }

  const auto pool{alloc.create_pool(device, initial_sets, pool_ratios)};
  if (!pool) {
    return tl::unexpected{pool.error()};
  }

  alloc.sets_per_pool = initial_sets + initial_sets / 2;

  alloc.ready_pools.push_back(*pool);

  return alloc;
}

void surge::renderer::vk::descriptor_allocator::destroy(VkDevice device) noexcept {
  for (auto &p : ready_pools) {
    vkDestroyDescriptorPool(device, p, get_alloc_callbacks());
  }

  ready_pools.clear();

  for (auto &p : full_pools) {
    vkDestroyDescriptorPool(device, p, get_alloc_callbacks());
  }

  full_pools.clear();
}

void surge::renderer::vk::descriptor_allocator::clear(VkDevice device) noexcept {
  for (auto &p : ready_pools) {
    vkResetDescriptorPool(device, p, 0);
  }

  for (auto &p : full_pools) {
    vkResetDescriptorPool(device, p, 0);
    ready_pools.push_back(p);
  }

  full_pools.clear();
}

auto surge::renderer::vk::descriptor_allocator::allocate(
    VkDevice device, VkDescriptorSetLayout layout,
    void *pNext) noexcept -> tl::expected<VkDescriptorSet, error> {

  // Get or create a pool to allocate from
  auto pool{get_pool(device)};

  if (!pool) {
    log_error("Unable to allocate descriptor set from pool");
    return tl::unexpected{pool.error()};
  }

  VkDescriptorSetAllocateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.pNext = pNext;
  info.descriptorPool = *pool;
  info.descriptorSetCount = 1;
  info.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  auto result{vkAllocateDescriptorSets(device, &info, &ds)};

  // allocation failed. Try again
  if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

    full_pools.push_back(*pool);

    pool = get_pool(device);
    if (!pool) {
      log_error("Unable to allocate descriptor set from pool. Allocation retry failed");
      return tl::unexpected{pool.error()};
    }

    info.descriptorPool = *pool;

    result = vkAllocateDescriptorSets(device, &info, &ds);

    if (result != VK_SUCCESS) {
      log_error("Unable to allocate descriptor sets from pool: {}", string_VkResult(result));
      return tl::unexpected{error::vk_descriptor_set_alloc};
    }
  }

  ready_pools.push_back(*pool);

  return ds;
}