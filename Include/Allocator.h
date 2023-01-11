#pragma once

#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

// #include VMA_VULKAN_VERSION 1002000

namespace AltheaEngine {

class Allocator {
public:
  Allocator(
      VkInstance instance,
      VkDevice device,
      VkPhysicalDevice physicalDevice);
  ~Allocator();

private:
  VmaAllocator _allocator;
};
} // namespace AltheaEngine