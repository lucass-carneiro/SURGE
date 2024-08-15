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

struct desc_pool_size_ratio {
  VkDescriptorType type{};
  float ratio{};
};

class descriptor_allocator {
private:
  vector<desc_pool_size_ratio> ratios{};
  vector<VkDescriptorPool> full_pools{};
  vector<VkDescriptorPool> ready_pools{};
  uint32_t sets_per_pool{0};

  auto get_pool(VkDevice device) noexcept -> tl::expected<VkDescriptorPool, error>;

  auto create_pool(VkDevice device, u32 set_count,
                   std::span<desc_pool_size_ratio> pool_ratios) noexcept
      -> tl::expected<VkDescriptorPool, error>;

  descriptor_allocator() noexcept = default;

public:
  static auto init(VkDevice device, uint32_t initial_sets,
                   std::span<desc_pool_size_ratio> pool_ratios) noexcept
      -> tl::expected<descriptor_allocator, error>;

  void destroy(VkDevice device) noexcept;

  void clear(VkDevice device) noexcept;

  auto allocate(VkDevice device, VkDescriptorSetLayout layout,
                void *pNext = nullptr) noexcept -> tl::expected<VkDescriptorSet, error>;
};

struct descriptor_layout {
  vector<VkDescriptorSetLayoutBinding> bindings;

  void add_binding(u32 binding, VkDescriptorType type) noexcept;
  void clear() noexcept;

  auto build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr,
             VkDescriptorSetLayoutCreateFlags flags
             = 0) noexcept -> tl::expected<VkDescriptorSetLayout, error>;
};

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_DESCRIPTOR_HPP