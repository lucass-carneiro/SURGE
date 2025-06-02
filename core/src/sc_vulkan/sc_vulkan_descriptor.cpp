#include "sc_vulkan/sc_vulkan_descriptor.hpp"

#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

void surge::renderer::vk::DescriptorLayoutBuilder::add_binding(u32 binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding newbind{};
  newbind.binding = binding;
  newbind.descriptorCount = 1;
  newbind.descriptorType = type;

  bindings.push_back(newbind);
}

void surge::renderer::vk::DescriptorLayoutBuilder::clear() { bindings.clear(); }

auto surge::renderer::vk::DescriptorLayoutBuilder::build(renderer::vk::Context ctx,
                                                         VkShaderStageFlags shader_stages,
                                                         void *pNext,
                                                         VkDescriptorSetLayoutCreateFlags flags)
    -> Result<VkDescriptorSetLayout> {

  for (auto &b : bindings) {
    b.stageFlags |= shader_stages;
  }

  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext = pNext;

  info.pBindings = bindings.data();
  info.bindingCount = static_cast<u32>(bindings.size());
  info.flags = flags;

  VkDescriptorSetLayout set_layout{};
  const auto result{
      vkCreateDescriptorSetLayout(ctx->device, &info, get_alloc_callbacks(), &set_layout)};

  if (result != VK_SUCCESS) {
    log_error("Unable create descriptor set layout: {}", string_VkResult(result));
    return Err{Error::vk_descriptor_set_layout_build};
  } else {
    return set_layout;
  }
}

void surge::renderer::vk::destroy_descriptor_set_layout(Context ctx, VkDescriptorSetLayout layout) {
  vkDestroyDescriptorSetLayout(ctx->device, layout, get_alloc_callbacks());
}

auto surge::renderer::vk::DescriptorPoolAllocator::init_pool(
    Context ctx, uint32_t max_sets, std::span<DescriptorPoolSizeRatio> pool_ratios)
    -> Result<void> {
  using std::ceil;

  containers::mimalloc::Vector<VkDescriptorPoolSize> pool_sizes{};

  for (const auto &ratio : pool_ratios) {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = ratio.type;
    pool_size.descriptorCount = static_cast<u32>(ceil(ratio.ratio * static_cast<float>(max_sets)));

    pool_sizes.push_back(pool_size);
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = max_sets;
  pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  const auto result{vkCreateDescriptorPool(ctx->device, &pool_info, get_alloc_callbacks(), &pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to create descriptor pool: {}", string_VkResult(result));
    return Err{Error::vk_descriptor_pool_init};
  } else {
    return {};
  }
}

auto surge::renderer::vk::DescriptorPoolAllocator::clear_descriptors(Context ctx) -> Result<void> {
  const auto result{vkResetDescriptorPool(ctx->device, pool, 0)};

  if (result != VK_SUCCESS) {
    log_error("Unable to reset descriptor pool: {}", string_VkResult(result));
    return Err{Error::vk_descriptor_pool_reset};
  } else {
    return {};
  }
}

void surge::renderer::vk::DescriptorPoolAllocator::destroy_pool(Context ctx) {
  vkDestroyDescriptorPool(ctx->device, pool, get_alloc_callbacks());
}

auto surge::renderer::vk::DescriptorPoolAllocator::allocate(Context ctx,
                                                            VkDescriptorSetLayout layout)
    -> Result<VkDescriptorSet> {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  const auto result{vkAllocateDescriptorSets(ctx->device, &allocInfo, &ds)};

  if (result != VK_SUCCESS) {
    log_error("Unable allocate descriptor sets: {}", string_VkResult(result));
    return Err{Error::vk_descriptor_set_alloc};
  } else {
    return ds;
  }
}

auto surge::renderer::vk::GrowableDescriptorAllocator::get_pool(Context ctx) -> VkDescriptorPool {
  VkDescriptorPool pool{};

  if (ready_pools.empty()) {
    pool = ready_pools.back();
    ready_pools.pop_back();
  } else {
    pool = create_pool(ctx, sets_per_pool, ratios);

    sets_per_pool = sets_per_pool * 2;
  }

  return pool;
}

auto surge::renderer::vk::GrowableDescriptorAllocator::create_pool(
    Context ctx, u32 num_sets, std::span<DescriptorPoolSizeRatio> pool_ratios) -> VkDescriptorPool {

  std::vector<VkDescriptorPoolSize> pool_sizes{};
  for (const auto &ratio : pool_ratios) {
    pool_sizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type,
        .descriptorCount = static_cast<u32>(ratio.ratio * static_cast<float>(num_sets))});
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = num_sets;
  pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  VkDescriptorPool pool;
  vkCreateDescriptorPool(ctx->device, &pool_info, get_alloc_callbacks(), &pool);
  return pool;
}

void surge::renderer::vk::GrowableDescriptorAllocator::init(
    Context ctx, u32 initial_sets, std::span<DescriptorPoolSizeRatio> pool_ratios) {
  ratios.clear();

  for (const auto &r : pool_ratios) {
    ratios.push_back(r);
  }

  const auto new_pool{create_pool(ctx, initial_sets, pool_ratios)};

  sets_per_pool = initial_sets * 2; // grow it next allocation

  ready_pools.push_back(new_pool);
}

void surge::renderer::vk::GrowableDescriptorAllocator::clear_pools(Context ctx) {
  for (auto &p : ready_pools) {
    vkResetDescriptorPool(ctx->device, p, 0);
  }
  for (auto p : full_pools) {
    vkResetDescriptorPool(ctx->device, p, 0);
    ready_pools.push_back(p);
  }
  full_pools.clear();
}

void surge::renderer::vk::GrowableDescriptorAllocator::destroy_pools(Context ctx) {
  for (auto p : ready_pools) {
    vkDestroyDescriptorPool(ctx->device, p, get_alloc_callbacks());
  }
  ready_pools.clear();
  for (auto p : full_pools) {
    vkDestroyDescriptorPool(ctx->device, p, get_alloc_callbacks());
  }
  full_pools.clear();
}

auto surge::renderer::vk::GrowableDescriptorAllocator::allocate(Context ctx,
                                                                VkDescriptorSetLayout layout,
                                                                void *pNext)
    -> Result<VkDescriptorSet> {
  // get or create a pool to allocate from
  auto poolToUse{get_pool(ctx)};

  VkDescriptorSetAllocateInfo dsai{};
  dsai.pNext = pNext;
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = poolToUse;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  auto result{vkAllocateDescriptorSets(ctx->device, &dsai, &ds)};

  if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
    full_pools.push_back(poolToUse);

    poolToUse = get_pool(ctx);
    dsai.descriptorPool = poolToUse;

    result = vkAllocateDescriptorSets(ctx->device, &dsai, &ds);
  }

  if (result != VK_SUCCESS) {
    log_error("Unable allocate descriptor sets: {}", string_VkResult(result));
    return Err{Error::vk_descriptor_set_alloc};
  }

  ready_pools.push_back(poolToUse);

  return ds;
}