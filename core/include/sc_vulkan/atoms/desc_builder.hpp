#ifndef SURGE_CORE_VULKAN_ATOM_DESC_BUILDER_HPP
#define SURGE_CORE_VULKAN_ATOM_DESC_BUILDER_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

#include <tl/expected.hpp>

namespace surge::vk_atom {

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

} // namespace surge::vk_atom

#endif // SURGE_CORE_VULKAN_ATOM_DESC_BUILDER_HPP