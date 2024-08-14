#ifndef SURGE_CORE_RENDERER_VK_DESCRIPTOR_HPP
#define SURGE_CORE_RENDERER_VK_DESCRIPTOR_HPP

#include "container_types.hpp"
#include "error_types.hpp"
#include "integer_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

#include <span>
#include <tl/expected.hpp>

namespace surge::renderer::vk {

struct descriptor_layout {
  vector<VkDescriptorSetLayoutBinding> bindings;

  void add_binding(u32 binding, VkDescriptorType type) noexcept;
  void clear() noexcept;

  auto build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr,
             VkDescriptorSetLayoutCreateFlags flags
             = 0) noexcept -> tl::expected<VkDescriptorSetLayout, error>;
};

struct descriptor_pool_size_ratio {
  VkDescriptorType type{};
  float ratio{};
};

auto create_pool(VkDevice device, u32 maxSets,
                 std::span<descriptor_pool_size_ratio> poolRatios) noexcept
    -> tl::expected<VkDescriptorPool, error>;

void reset_pool(VkDevice device, VkDescriptorPool pool) noexcept;

void destroy_pool(VkDevice device, VkDescriptorPool pool) noexcept;

auto alloc_from_pool(VkDevice device, VkDescriptorPool pool,
                     VkDescriptorSetLayout layout) noexcept -> tl::expected<VkDescriptorSet, error>;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_DESCRIPTOR_HPP