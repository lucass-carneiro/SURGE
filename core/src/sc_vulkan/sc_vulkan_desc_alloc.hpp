#ifndef SURGE_RENDERER_VULKAN_DESCRIPTOR_ALLOCATOR_HPP
#define SURGE_RENDERER_VULKAN_DESCRIPTOR_ALLOCATOR_HPP

#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

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

class desc_alloc {
private:
  VkDescriptorPool pool{};

public:
  auto init_pool(VkDevice device, u32 max_sets, std::span<desc_pool_size_ratio> pool_ratios)
      -> tl::expected<void, error>;

  auto clear_descriptors(VkDevice device) -> tl::expected<void, error>;

  void destroy_pool(VkDevice device);

  auto allocate(VkDevice device, VkDescriptorSetLayout layout)
      -> tl::expected<VkDescriptorSet, error>;
};

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_DESCRIPTOR_ALLOCATOR_HPP
