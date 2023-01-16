#pragma once

#include "UniqueVkHandle.h"
#include "vk_mem_alloc.h"

#include <vulkan/vulkan.h>

// #include VMA_VULKAN_VERSION 1002000

namespace AltheaEngine {
class BufferAllocation {
public:
  VkBuffer getBuffer() const { return this->_buffer; }

  const VmaAllocationInfo& getInfo() const { return this->_info; }

  void* mapMemory() const;
  void unmapMemory() const;

private:
  friend class Allocator;

  struct BufferDeleter {
    VmaAllocator allocator;
    VmaAllocation allocation;

    void operator()(VkDevice device, VkBuffer buffer);
  };

  UniqueVkHandle<VkBuffer, BufferDeleter> _buffer;

  VmaAllocator _allocator;
  VmaAllocation _allocation;

  VmaAllocationInfo _info{};
};

class ImageAllocation {
public:
  VkImage getImage() const { return this->_image; }

  const VmaAllocationInfo& getInfo() const { return this->_info; }

  void* mapMemory() const;
  void unmapMemory() const;

private:
  friend class Allocator;

  struct ImageDeleter {
    VmaAllocator allocator;
    VmaAllocation allocation;

    void operator()(VkDevice device, VkImage image);
  };

  UniqueVkHandle<VkImage, ImageDeleter> _image;

  VmaAllocator _allocator;
  VmaAllocation _allocation;

  VmaAllocationInfo _info{};
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