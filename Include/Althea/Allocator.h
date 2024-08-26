#pragma once

#include "Library.h"
#include "UniqueVkHandle.h"
#include "vk_mem_alloc.h"

#include <vulkan/vulkan.h>

// #include VMA_VULKAN_VERSION 1002000

namespace AltheaEngine {
#if _DEBUG
struct AllocatorOverride {
public:
  typedef void*(MallocOverrideFuncType)(size_t);
  typedef void(FreeOverrideFuncType)(void*);

  thread_local static MallocOverrideFuncType* g_mallocOverride;
  thread_local static FreeOverrideFuncType* g_freeOverride;

  AllocatorOverride(
      MallocOverrideFuncType mallocOverride,
      FreeOverrideFuncType freeOverride);
  ~AllocatorOverride();

private:
  MallocOverrideFuncType* m_prevMallocOverride;
  FreeOverrideFuncType* m_prevFreeOverride;
};

#define SCOPED_ALLOCATOR_OVERRIDE(mallocOverride, freeOverride)                \
  AllocatorOverride __allocOverride(mallocOverride, freeOverride);

struct LinearAllocator {
public:
  thread_local static char* g_pAlloc;
  thread_local static size_t g_allocSize;

  static constexpr uint32_t MAX_ALLOC_SIZE = 100000;

  static void* allocate(const size_t n);
  static void deallocate(void* p);

  LinearAllocator();
  ~LinearAllocator();

private:
  AllocatorOverride m_override;
};

#define SCOPED_LINEAR_ALLOCATOR                                                \
  LinearAllocator __scopedOverrideAllocator = LinearAllocator();

#endif // _DEBUG

class ALTHEA_API BufferAllocation {
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

class ALTHEA_API ImageAllocation {
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

enum class ALTHEA_API AllocationType { CPU_GPU, CPU_GPU_STAGING, GPU_ONLY };

class ALTHEA_API Allocator {
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