#include "Allocator.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <stdexcept>

// override allocator in debug builds
#ifdef _DEBUG
namespace AltheaEngine {
thread_local AllocatorOverride::MallocOverrideFuncType*
    AllocatorOverride::g_mallocOverride = nullptr;
thread_local AllocatorOverride::FreeOverrideFuncType*
    AllocatorOverride::g_freeOverride = nullptr;

AllocatorOverride::AllocatorOverride(
    MallocOverrideFuncType mallocOverride,
    FreeOverrideFuncType freeOverride)
    : m_prevMallocOverride(g_mallocOverride),
      m_prevFreeOverride(g_freeOverride) {
  g_mallocOverride = mallocOverride;
  g_freeOverride = freeOverride;
}

AllocatorOverride::~AllocatorOverride() {
  g_mallocOverride = m_prevMallocOverride;
  g_freeOverride = m_prevFreeOverride;
}

/*static*/
void* LinearAllocator::allocate(const size_t n) {
  assert(g_allocSize + n <= MAX_ALLOC_SIZE);
  assert(g_pAlloc);

  size_t oldSize = g_allocSize;
  g_allocSize += n;

  return &g_pAlloc[oldSize];
}

// WARNING DEALLOCATIONS ARE IGNORED
/*static*/
void LinearAllocator::deallocate(void* p) {}

LinearAllocator::LinearAllocator() : m_override(allocate, deallocate) {
  assert(!g_pAlloc);
  assert(g_allocSize == 0);

  g_pAlloc = (char*)malloc(MAX_ALLOC_SIZE);
  g_allocSize = 0;
}

LinearAllocator::~LinearAllocator() {
  free(g_pAlloc);
  g_pAlloc = nullptr;
  g_allocSize = 0;
}

thread_local char* LinearAllocator::g_pAlloc = nullptr;
thread_local size_t LinearAllocator::g_allocSize = 0;

} // namespace AltheaEngine
#include <new>
void operator delete(void* p) noexcept {
  if (!AltheaEngine::AllocatorOverride::g_freeOverride)
    free(p);
  else
    AltheaEngine::AllocatorOverride::g_freeOverride(p);
}
void operator delete[](void* p) noexcept {
  if (!AltheaEngine::AllocatorOverride::g_freeOverride)
    free(p);
  else
    AltheaEngine::AllocatorOverride::g_freeOverride(p);
}

void* operator new(std::size_t n) noexcept(false) {
  if (!AltheaEngine::AllocatorOverride::g_mallocOverride)
    return malloc(n);
  else
    return AltheaEngine::AllocatorOverride::g_mallocOverride(n);
}

void* operator new[](std::size_t n) noexcept(false) {
  if (!AltheaEngine::AllocatorOverride::g_mallocOverride)
    return malloc(n);
  else
    return AltheaEngine::AllocatorOverride::g_mallocOverride(n);
}

void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept {
  if (!AltheaEngine::AllocatorOverride::g_mallocOverride)
    return malloc(n);
  else
    return AltheaEngine::AllocatorOverride::g_mallocOverride(n);
}
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept {
  if (!AltheaEngine::AllocatorOverride::g_mallocOverride)
    return malloc(n);
  else
    return AltheaEngine::AllocatorOverride::g_mallocOverride(n);
}

#endif // _DEBUG

// TODO: provide a bit more abstraction here once various allocation types can
// be reasonably categorized (e.g., staging, host-device-visible,
// host-sequential-write-device-visible, device-local) Ideally avoid e.g.,
// allocateBuffer(...) requiring VmaAllocationCreateInfo
namespace AltheaEngine {
void BufferAllocation::BufferDeleter::operator()(
    VkDevice device,
    VkBuffer buffer) {
  vmaDestroyBuffer(this->allocator, buffer, this->allocation);
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

void ImageAllocation::ImageDeleter::operator()(VkDevice device, VkImage image) {
  vmaDestroyImage(this->allocator, image, this->allocation);
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
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  // | VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
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

  VkBuffer buffer;
  if (vmaCreateBuffer(
          this->_allocator,
          &bufferInfo,
          &allocInfo,
          &buffer,
          &result._allocation,
          &result._info) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer.");
  }

  result._buffer.set(
      VK_NULL_HANDLE,
      buffer,
      {this->_allocator, result._allocation});

  return result;
}

ImageAllocation Allocator::createImage(
    const VkImageCreateInfo& imageInfo,
    const VmaAllocationCreateInfo& allocInfo) const {
  ImageAllocation result;
  result._allocator = this->_allocator;

  VkImage image;
  if (vmaCreateImage(
          this->_allocator,
          &imageInfo,
          &allocInfo,
          &image,
          &result._allocation,
          &result._info)) {
    throw std::runtime_error("Failed to allocate image.");
  }

  result._image.set(
      VK_NULL_HANDLE,
      image,
      {this->_allocator, result._allocation});

  return result;
}
} // namespace AltheaEngine