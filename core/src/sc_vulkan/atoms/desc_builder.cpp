#include "sc_vulkan/atoms/desc_builder.hpp"

#include "../sc_vulkan_malloc.hpp"
#include "sc_logging.hpp"

#include <vulkan/vk_enum_string_helper.h>

void surge::vk_atom::desc_builder::add_binding(u32 binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding b{};
  b.binding = binding;
  b.descriptorCount = 1;
  b.descriptorType = type;

  bindings.push_back(b);
}

void surge::vk_atom::desc_builder::clear() { bindings.clear(); }

auto surge::vk_atom::desc_builder::build(VkDevice device, VkShaderStageFlags shader_stages,
                                         void *pNext, VkDescriptorSetLayoutCreateFlags flags)
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
      vkCreateDescriptorSetLayout(device, &info, renderer::vk::get_alloc_callbacks(), &set)};

  if (result != VK_SUCCESS) {
    log_error("Unable to create descriptor set layout: {}", string_VkResult(result));
    return tl::unexpected{vk_descriptor_set_layout_build};
  }

  return set;
}