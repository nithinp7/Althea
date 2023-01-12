#pragma once

#include "vk_mem_alloc.h"

#include <vulkan/vulkan.h>

// #include VMA_VULKAN_VERSION 1002000

namespace AltheaEngine {
class BufferAllocation {
public:
  // Move-only semantics
  BufferAllocation() = default;
  BufferAllocation(BufferAllocation&& rhs);
  BufferAllocation(const BufferAllocation& rhs) = delete;

  BufferAllocation& operator=(BufferAllocation&& rhs);
  BufferAllocation& operator=(const BufferAllocation& rhs) = delete;

  // Underlying allocation is automatically released to the allocator when
  // this object is destroyed.
  ~BufferAllocation();

  VkBuffer getBuffer() const { return this->_buffer; }

  VmaAllocation getAllocation() const { return this->_allocation; }

  const VmaAllocationInfo& getInfo() const { return this->_info; }

  void* mapMemory() const;
  void unmapMemory() const;

private:
  friend class Allocator;

  VkBuffer _buffer = VK_NULL_HANDLE;
  VmaAllocation _allocation = VK_NULL_HANDLE;
  VmaAllocationInfo _info{};

  VmaAllocator _allocator = VK_NULL_HANDLE;
};

class ImageAllocation {
public:
  // Move-only semantics
  ImageAllocation() = default;
  ImageAllocation(ImageAllocation&& rhs);
  ImageAllocation(const ImageAllocation& rhs) = delete;

  ImageAllocation& operator=(ImageAllocation&& rhs);
  ImageAllocation& operator=(const ImageAllocation& rhs) = delete;

  // Underlying allocation is automatically released to the allocator when
  // this object is destroyed.
  ~ImageAllocation();

  VkImage getImage() const { return this->_image; }

  VmaAllocation getAllocation() const { return this->_allocation; }

  const VmaAllocationInfo& getInfo() const { return this->_info; }

  void* mapMemory() const;
  void unmapMemory() const;

private:
  friend class Allocator;

  VkImage _image = VK_NULL_HANDLE;
  VmaAllocation _allocation = VK_NULL_HANDLE;
  VmaAllocationInfo _info{};

  VmaAllocator _allocator = VK_NULL_HANDLE;
};

enum class AllocationType { CPU_GPU, CPU_GPU_STAGING, GPU_ONLY };

class Allocator {
public:
  Allocator(
      VkInstance instance,
      VkDevice device,
      VkPhysicalDevice physicalDevice);
  ~Allocator();

  BufferAllocation createBuffer(
      const VkBufferCreateInfo& bufferInfo,
      const VmaAllocationCreateInfo& allocInfo) const;
  ImageAllocation createImage(
      const VkImageCreateInfo& imageInfo,
      const VmaAllocationCreateInfo& allocInfo) const;
private:
  VmaAllocator _allocator;
};
} // namespace AltheaEngine