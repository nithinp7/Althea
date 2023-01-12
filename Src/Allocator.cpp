#include "Allocator.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <stdexcept>

// TODO: provide a bit more abstraction here once various allocation types can be reasonably
// categorized (e.g., staging, host-device-visible, host-sequential-write-device-visible, device-local)
// Ideally avoid e.g., allocateBuffer(...) requiring VmaAllocationCreateInfo
namespace AltheaEngine {
BufferAllocation::BufferAllocation(BufferAllocation&& rhs) 
    : _buffer(rhs._buffer),
      _allocation(rhs._allocation),
      _info(rhs._info),
      _allocator(rhs._allocator) {
  rhs._buffer = VK_NULL_HANDLE;
  rhs._allocation = VK_NULL_HANDLE;
  rhs._info = {};
  rhs._allocator = VK_NULL_HANDLE;
}

BufferAllocation& BufferAllocation::operator=(BufferAllocation&& rhs) {
  this->_buffer = rhs._buffer;
  this->_allocation = rhs._allocation;
  this->_info = rhs._info;
  this->_allocator = rhs._allocator;

  rhs._buffer = VK_NULL_HANDLE;
  rhs._allocation = VK_NULL_HANDLE;
  rhs._info = {};
  rhs._allocator = VK_NULL_HANDLE;
} 

BufferAllocation::~BufferAllocation() {
  if (this->_buffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(this->_allocator, this->_buffer, this->_allocation);
  }
}

// TODO: provide way to automate / enforce write-combined, e.g., wrap:
// map --> memcpy --> unmap, all in one shot so random writes are avoided.
void* BufferAllocation::mapMemory() const {
  void* pDest;
  if (vmaMapMemory(this->_allocator, this->_allocation, &pDest) != VK_SUCCESS) {
    throw std::runtime_error("Failed to complete VMA memory mapping.");
  }

  return pDest;
}

void BufferAllocation::unmapMemory() const {
  vmaUnmapMemory(this->_allocator, this->_allocation);
}

ImageAllocation::ImageAllocation(ImageAllocation&& rhs) 
    : _image(rhs._image),
      _allocation(rhs._allocation),
      _info(rhs._info),
      _allocator(rhs._allocator) {
  rhs._image = VK_NULL_HANDLE;
  rhs._allocation = VK_NULL_HANDLE;
  rhs._info = {};
  rhs._allocator = VK_NULL_HANDLE;
}

ImageAllocation& ImageAllocation::operator=(ImageAllocation&& rhs) {
  this->_image = rhs._image;
  this->_allocation = rhs._allocation;
  this->_info = rhs._info;
  this->_allocator = rhs._allocator;

  rhs._image = VK_NULL_HANDLE;
  rhs._allocation = VK_NULL_HANDLE;
  rhs._info = {};
  rhs._allocator = VK_NULL_HANDLE;
} 

ImageAllocation::~ImageAllocation() {
  if (this->_image != VK_NULL_HANDLE) {
    vmaDestroyImage(this->_allocator, this->_image, this->_allocation);
  }
}

void* ImageAllocation::mapMemory() const {
  void* pDest;
  if (vmaMapMemory(this->_allocator, this->_allocation, &pDest) != VK_SUCCESS) {
    throw std::runtime_error("Failed to complete VMA memory mapping.");
  }

  return pDest;
}

void ImageAllocation::unmapMemory() const {
  vmaUnmapMemory(this->_allocator, this->_allocation);
}

Allocator::Allocator(
    VkInstance instance,
    VkDevice device,
    VkPhysicalDevice physicalDevice) {
  VmaAllocatorCreateInfo allocatorInfo{};
  // TODO: Should allocations be synchronized externally instead?
  allocatorInfo.flags = 0; // VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
  allocatorInfo.physicalDevice = physicalDevice;
  allocatorInfo.device = device;
  allocatorInfo.instance = instance;
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  // TODO: Useful for measuring memory usage
  // allocatorInfo.pAllocationCallbacks = nullptr;
  // allocatorInfo.pDeviceMemoryCallbacks = nullptr;

  if (vmaCreateAllocator(&allocatorInfo, &this->_allocator) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan Memory Allocator.");
  }
}

Allocator::~Allocator() { vmaDestroyAllocator(this->_allocator); }

BufferAllocation Allocator::createBuffer(
    const VkBufferCreateInfo& bufferInfo,
    const VmaAllocationCreateInfo& allocInfo) const {
  BufferAllocation result;
  result._allocator = this->_allocator;

  if (vmaCreateBuffer(
          this->_allocator,
          &bufferInfo,
          &allocInfo,
          &result._buffer,
          &result._allocation,
          &result._info) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer.");
  }

  return result;
}

ImageAllocation Allocator::createImage(
    const VkImageCreateInfo& imageInfo,
    const VmaAllocationCreateInfo& allocInfo) const {
  ImageAllocation result;
  result._allocator = this->_allocator;

  if (vmaCreateImage(
          this->_allocator,
          &imageInfo,
          &allocInfo,
          &result._image,
          &result._allocation,
          &result._info)) {
    throw std::runtime_error("Failed to allocate image.");
  }

  return result;
}
} // namespace AltheaEngine