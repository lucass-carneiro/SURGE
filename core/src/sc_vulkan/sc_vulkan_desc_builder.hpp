#ifndef SURGE_RENDERER_VULKAN_DESCRIPTOR_BUILDER_HPP
#define SURGE_RENDERER_VULKAN_DESCRIPTOR_BUILDER_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

#include <tl/expected.hpp>

namespace surge::renderer::vk {

class desc_builder {
private:
  vector<VkDescriptorSetLayoutBinding> bindings;

public:
  void add_binding(u32 binding, VkDescriptorType type);
  void clear();
  auto build(VkDevice device, VkShaderStageFlags shader_stages, void *pNext = nullptr,
             VkDescriptorSetLayoutCreateFlags flags = 0)
      -> tl::expected<VkDescriptorSetLayout, error>;
};

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_DESCRIPTOR_BUILDER_HPP