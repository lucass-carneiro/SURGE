#ifndef SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP
#define SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"
#include "sc_vulkan/sc_vulkan.hpp"

#include <span>

namespace surge::renderer::vk {

class DescriptorLayoutBuilder {
private:
  containers::mimalloc::Vector<VkDescriptorSetLayoutBinding> bindings;

public:
  void add_binding(u32 binding, VkDescriptorType type);
  void clear();
  auto build(Context ctx, VkShaderStageFlags shader_stages, void *pNext = nullptr,
             VkDescriptorSetLayoutCreateFlags flags = 0) -> Result<VkDescriptorSetLayout>;
};

void destroy_descriptor_set_layout(Context ctx, VkDescriptorSetLayout layout);

struct DescriptorPoolSizeRatio {
  VkDescriptorType type;
  float ratio;
};

class DescriptorPoolAllocator {
private:
  VkDescriptorPool pool;

public:
  auto init_pool(Context ctx, uint32_t max_sets,
                 std::span<DescriptorPoolSizeRatio> pool_ratios) -> Result<void>;
  auto clear_descriptors(Context ctx) -> Result<void>;
  void destroy_pool(Context ctx);

  auto allocate(Context ctx, VkDescriptorSetLayout layout) -> Result<VkDescriptorSet>;
};

} // namespace surge::renderer::vk

#endif // SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP