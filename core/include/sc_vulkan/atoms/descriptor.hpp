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

struct pool_size_ratio {
  VkDescriptorType type{};
  float ratio{};
};

class allocator {
private:
  static constexpr u32 max_sets_per_pool{4096};
  u32 sets_per_pool{0};

  vector<pool_size_ratio> ratios{};
  vector<VkDescriptorPool> full_pools{};
  vector<VkDescriptorPool> ready_pools{};

  auto get_pool(VkDevice device) -> VkDescriptorPool;

  auto create_pool(VkDevice device, u32 set_count, std::span<pool_size_ratio> pool_ratios)
      -> VkDescriptorPool;

public:
  void init(renderer::vk::context ctx, u32 initial_sets, std::span<pool_size_ratio> pool_ratios);

  void clear_pools(renderer::vk::context ctx);

  void destroy_pools(renderer::vk::context ctx);

  auto allocate(renderer::vk::context ctx, VkDescriptorSetLayout layout, void *pNext = nullptr)
      -> tl::expected<VkDescriptorSet, error>;
};

class writer {
private:
  deque<VkDescriptorBufferInfo> buffer_infos{};
  deque<VkDescriptorImageInfo> image_infos{};
  vector<VkWriteDescriptorSet> writes{};

public:
  void write_buffer(u32 binding, VkBuffer buffer, size_t size, size_t offset,
                    VkDescriptorType type);

  void write_image(u32 binding, VkImageView image, VkSampler sampler, VkImageLayout layout,
                   VkDescriptorType type);

  void clear();

  void update_set(renderer::vk::context ctx, VkDescriptorSet set);
};

class builder {
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

} // namespace surge::vk_atom::descriptor

#endif // SURGE_CORE_VULKAN_ATOM_DESCRIPTOR_HPP