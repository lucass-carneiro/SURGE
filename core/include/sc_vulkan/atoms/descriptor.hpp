#ifndef SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP
#define SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"
#include "sc_vulkan/sc_vulkan.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

#include <span>
#include <tl/expected.hpp>

namespace surge::vk_atom::descriptor {

struct desc_pool_size_ratio {
  VkDescriptorType type{};
  float ratio{};
};

class desc_alloc {
private:
  VkDescriptorPool pool{};

public:
  auto init_pool(renderer::vk::context ctx, u32 max_sets,
                 std::span<desc_pool_size_ratio> pool_ratios) -> tl::expected<void, error>;

  auto clear_descriptors(renderer::vk::context ctx) -> tl::expected<void, error>;

  void destroy_pool(renderer::vk::context ctx);

  auto allocate(renderer::vk::context ctx, VkDescriptorSetLayout layout)
      -> tl::expected<VkDescriptorSet, error>;
};

class desc_builder {
private:
  vector<VkDescriptorSetLayoutBinding> bindings;

public:
  void add_binding(u32 binding, VkDescriptorType type);
  void clear();
  auto build(renderer::vk::context ctx, VkShaderStageFlags shader_stages, void *pNext = nullptr,
             VkDescriptorSetLayoutCreateFlags flags = 0)
      -> tl::expected<VkDescriptorSetLayout, error>;
};

auto draw_img_desc_info(renderer::vk::context ctx) -> VkDescriptorImageInfo;
void destroy_desc_layout(renderer::vk::context ctx, VkDescriptorSetLayout layout);

void update_desc_sets(renderer::vk::context ctx, u32 write_count,
                      const VkWriteDescriptorSet *desc_writes, u32 copy_count,
                      const VkCopyDescriptorSet *desc_copies);

} // namespace surge::vk_atom::descriptor

#endif // SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP