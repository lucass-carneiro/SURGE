#include "sc_vulkan/atoms/descriptor.hpp"

#include "../sc_vulkan_malloc.hpp"
#include "../sc_vulkan_types.hpp"
#include "sc_container_types.hpp"
#include "sc_logging.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::vk_atom::descriptor::allocator::get_pool(VkDevice device) -> VkDescriptorPool {
  VkDescriptorPool new_pool{};

  if (ready_pools.size() != 0) {
    new_pool = ready_pools.back();
    ready_pools.pop_back();
  } else {
    new_pool = create_pool(device, sets_per_pool, ratios);

    sets_per_pool *= 2;

    if (sets_per_pool > max_sets_per_pool) {
      log_info("Max descriptor sets per pool limit ({}) reached. Ignoring resize request",
               max_sets_per_pool);
      sets_per_pool = max_sets_per_pool;
    }
  }

  return new_pool;
}

auto surge::vk_atom::descriptor::allocator::create_pool(VkDevice device, u32 set_count,
                                                        std::span<pool_size_ratio> pool_ratios)
    -> VkDescriptorPool {
  vector<VkDescriptorPoolSize> poolSizes{};

  for (const auto &ratio : pool_ratios) {
    poolSizes.push_back(VkDescriptorPoolSize{
        .type = ratio.type, .descriptorCount = static_cast<u32>(ratio.ratio * set_count)});
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = set_count;
  pool_info.poolSizeCount = static_cast<u32>(poolSizes.size());
  pool_info.pPoolSizes = poolSizes.data();

  VkDescriptorPool new_pool{};
  vkCreateDescriptorPool(device, &pool_info, nullptr, &new_pool);
  return new_pool;
}

void surge::vk_atom::descriptor::allocator::init(renderer::vk::context ctx, u32 initial_sets,
                                                 std::span<pool_size_ratio> pool_ratios) {
  ratios.clear();

  for (auto &r : pool_ratios) {
    ratios.push_back(r);
  }

  auto new_pool{create_pool(ctx->device, initial_sets, pool_ratios)};
  sets_per_pool = initial_sets * 2;
  ready_pools.push_back(new_pool);
}

void surge::vk_atom::descriptor::allocator::clear_pools(renderer::vk::context ctx) {
  for (auto &p : ready_pools) {
    vkResetDescriptorPool(ctx->device, p, 0);
  }

  for (auto &p : full_pools) {
    vkResetDescriptorPool(ctx->device, p, 0);
    ready_pools.push_back(p);
  }

  full_pools.clear();
}

void surge::vk_atom::descriptor::allocator::destroy_pools(renderer::vk::context ctx) {
  for (auto &p : ready_pools) {
    vkDestroyDescriptorPool(ctx->device, p, nullptr);
  }

  ready_pools.clear();

  for (auto &p : full_pools) {
    vkDestroyDescriptorPool(ctx->device, p, nullptr);
  }

  full_pools.clear();
}

auto surge::vk_atom::descriptor::allocator::allocate(renderer::vk::context ctx,
                                                     VkDescriptorSetLayout layout, void *pNext)
    -> tl::expected<VkDescriptorSet, error> {

  // Get or create a pool to allocate from
  auto pool_to_use{get_pool(ctx->device)};

  VkDescriptorSetAllocateInfo ai{};
  ai.pNext = pNext;
  ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  ai.descriptorPool = pool_to_use;
  ai.descriptorSetCount = 1;
  ai.pSetLayouts = &layout;

  VkDescriptorSet ds{};
  auto result{vkAllocateDescriptorSets(ctx->device, &ai, &ds)};

  // allocation failed. Try again
  if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
    full_pools.push_back(pool_to_use);

    pool_to_use = get_pool(ctx->device);
    ai.descriptorPool = pool_to_use;

    result = vkAllocateDescriptorSets(ctx->device, &ai, &ds);

    if (result != VK_SUCCESS) {
      log_error("Unable to allocate descriptor sets: {}", string_VkResult(result));
      return tl::unexpected{vk_descriptor_set_alloc};
    }
  }

  ready_pools.push_back(pool_to_use);

  return ds;
}

void surge::vk_atom::descriptor::writer::write_buffer(u32 binding, VkBuffer buffer, size_t size,
                                                      size_t offset, VkDescriptorType type) {
  auto &info{buffer_infos.emplace_back(
      VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size})};

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;
  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE; // Left empty for now until we need to write it
  write.descriptorCount = 1;
  write.descriptorType = type;
  write.pBufferInfo = &info;

  writes.push_back(write);
}

void surge::vk_atom::descriptor::writer::write_image(u32 binding, VkImageView image,
                                                     VkSampler sampler, VkImageLayout layout,
                                                     VkDescriptorType type) {
  auto &info = image_infos.emplace_back(
      VkDescriptorImageInfo{.sampler = sampler, .imageView = image, .imageLayout = layout});

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;
  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
  write.descriptorCount = 1;
  write.descriptorType = type;
  write.pImageInfo = &info;

  writes.push_back(write);
}

void surge::vk_atom::descriptor::writer::clear() {
  buffer_infos.clear();
  image_infos.clear();
  writes.clear();
}

void surge::vk_atom::descriptor::writer::update_set(renderer::vk::context ctx,
                                                    VkDescriptorSet set) {
  for (auto &write : writes) {
    write.dstSet = set;
  }

  vkUpdateDescriptorSets(ctx->device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
}

auto surge::vk_atom::descriptor::draw_img_desc_info(renderer::vk::context ctx)
    -> VkDescriptorImageInfo {
  VkDescriptorImageInfo info{};
  info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  info.imageView = ctx->draw_image.image_view;

  return info;
}

void surge::vk_atom::descriptor::builder::add_binding(u32 binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding b{};
  b.binding = binding;
  b.descriptorCount = 1;
  b.descriptorType = type;

  bindings.push_back(b);
}

void surge::vk_atom::descriptor::builder::clear() { bindings.clear(); }

auto surge::vk_atom::descriptor::builder::build(renderer::vk::context ctx,
                                                VkShaderStageFlags shader_stages, void *pNext,
                                                VkDescriptorSetLayoutCreateFlags flags)
    -> tl::expected<VkDescriptorSetLayout, error> {
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
