#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::create_buffer(Context ctx, size_t size, VkBufferUsageFlags usage_flags,
                                        VmaMemoryUsage memory_usage) -> Result<Buffer> {
  // allocate buffer
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext = nullptr;
  buffer_info.size = size;
  buffer_info.usage = usage_flags;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = memory_usage;
  alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  Buffer buffer{};

  // allocate the buffer
  const auto result{vmaCreateBuffer(ctx->allocator, &buffer_info, &alloc_info, &buffer.buffer,
                                    &buffer.allocation, &buffer.info)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate buffer: {}", string_VkResult(result));
    return Err{Error::vk_buffer_allocation};
  }

  return buffer;
}

void surge::renderer::vk::destroy_buffer(Context ctx, Buffer buff) {
  vmaDestroyBuffer(ctx->allocator, buff.buffer, buff.allocation);
}