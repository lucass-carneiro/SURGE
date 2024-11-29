#define VMA_IMPLEMENTATION

#include "renderer_vk_malloc.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "options.hpp"

#include <vulkan/vk_enum_string_helper.h>

static auto vk_malloc(void *, size_t size, size_t alignment,
                      [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_alloc(size, alignment);
}

static auto vk_realloc(void *, void *pOriginal, size_t size, size_t alignment,
                       [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_realloc(pOriginal, size, alignment);
}

static void vk_free(void *, void *pMemory) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory free event");
#endif
  surge::allocators::mimalloc::free(pMemory);
}

static void vk_internal_malloc([[maybe_unused]] void *, [[maybe_unused]] size_t size,
                               [[maybe_unused]] VkInternalAllocationType type,
                               [[maybe_unused]] VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc event:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static void vk_internal_free([[maybe_unused]] void *, [[maybe_unused]] size_t size,
                             [[maybe_unused]] VkInternalAllocationType type,
                             [[maybe_unused]] VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc free:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static const VkAllocationCallbacks alloc_callbacks{.pUserData = nullptr,
                                                   .pfnAllocation = vk_malloc,
                                                   .pfnReallocation = vk_realloc,
                                                   .pfnFree = vk_free,
                                                   .pfnInternalAllocation = vk_internal_malloc,
                                                   .pfnInternalFree = vk_internal_free};

auto surge::renderer::vk::get_alloc_callbacks() noexcept -> const VkAllocationCallbacks * {
  return &alloc_callbacks;
}

auto surge::renderer::vk::create_memory_allocator(VkInstance instance, VkPhysicalDevice phys_dev,
                                                  VkDevice logi_dev) noexcept
    -> tl::expected<VmaAllocator, error> {
  log_info("Creating memory allocator");

  VmaAllocatorCreateInfo alloc_info{};
  alloc_info.physicalDevice = phys_dev;
  alloc_info.device = logi_dev;
  alloc_info.instance = instance;
  alloc_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  VmaAllocator allocator{};
  const auto result{vmaCreateAllocator(&alloc_info, &allocator)};

  if (result != VK_SUCCESS) {
    log_error("Unable create memory allocator: {}", string_VkResult(result));
    return tl::unexpected{error::vk_allocator_creation};
  }

  log_info("Memory allocator created");
  return allocator;
}