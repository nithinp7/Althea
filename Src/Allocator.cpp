#include "Allocator.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <stdexcept>

namespace AltheaEngine {

Allocator::Allocator(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice) {
  VmaAllocatorCreateInfo allocatorInfo;
  // TODO: Should allocations be synchronized externally instead?  
  allocatorInfo.flags = 0;//VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
  allocatorInfo.physicalDevice = physicalDevice;
  allocatorInfo.device = device;
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  // TODO: Useful for measuring memory usage
  allocatorInfo.pAllocationCallbacks = nullptr;
  allocatorInfo.pDeviceMemoryCallbacks = nullptr; 

  if (vmaCreateAllocator(&allocatorInfo, &this->_allocator) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan Memory Allocator.");
  }
}

Allocator::~Allocator() {
  vmaDestroyAllocator(this->_allocator);
}
} // namespace AltheaEngine