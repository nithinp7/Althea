#include "DynamicBuffer.h"

#include "Application.h"

namespace AltheaEngine {

DynamicBuffer::DynamicBuffer(DynamicBuffer&& rhs)
    : _bufferSize(rhs._bufferSize),
      _allocation(std::move(rhs._allocation)),
      _pMappedMemory(rhs._pMappedMemory) {
  rhs._pMappedMemory = nullptr;
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    this->_indices[i] = rhs._indices[i];
}

void DynamicBuffer::registerToHeap(GlobalHeap& heap) {
  // Register bindless indices on the heap for these buffer regions
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    this->_indices[i] = heap.registerBuffer();
    heap.updateStorageBuffer(
        this->_indices[i],
        this->_allocation.getBuffer(),
        i * this->_bufferSize,
        this->_bufferSize);
  }
}

void DynamicBuffer::zeroBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t ringBufferIndex) {
  vkCmdFillBuffer(
      commandBuffer,
      getAllocation().getBuffer(),
      ringBufferIndex * _bufferSize,
      _bufferSize,
      0);
}

void DynamicBuffer::zeroAllBuffers(VkCommandBuffer commandBuffer) {
  vkCmdFillBuffer(
      commandBuffer,
      getAllocation().getBuffer(),
      0,
      MAX_FRAMES_IN_FLIGHT * _bufferSize,
      0);
}

DynamicBuffer& DynamicBuffer::operator=(DynamicBuffer&& rhs) {
  if (this->_pMappedMemory) {
    this->_allocation.unmapMemory();
  }

  this->_bufferSize = rhs._bufferSize;
  this->_allocation = std::move(rhs._allocation);
  this->_pMappedMemory = rhs._pMappedMemory;
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    this->_indices[i] = rhs._indices[i];

  rhs._pMappedMemory = nullptr;

  return *this;
}

DynamicBuffer::DynamicBuffer(
    const Application& app,
    VkBufferUsageFlags usage,
    size_t bufferSize,
    size_t offsetAlignment)
    : _bufferSize(
          bufferSize +
          // Add padding to enforce alignment
          (offsetAlignment - bufferSize % offsetAlignment) % offsetAlignment) {
  // Create double buffered persistently mapped region
  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  this->_allocation = BufferUtilities::createBuffer(
      app,
      this->_bufferSize * MAX_FRAMES_IN_FLIGHT,
      usage,
      deviceAllocInfo);

  this->_pMappedMemory =
      reinterpret_cast<std::byte*>(this->_allocation.mapMemory());
}

DynamicBuffer::~DynamicBuffer() {
  if (this->_pMappedMemory) {
    this->_allocation.unmapMemory();
  }
}

void DynamicBuffer::updateData(
    uint32_t ringBufferIndex,
    gsl::span<const std::byte> data) {
  if (data.size() > this->_bufferSize) {
    throw std::runtime_error("Attempting to update DynamicBuffer with "
                             "incorrect buffer size.");
  }

  size_t bufferOffset = ringBufferIndex * this->_bufferSize;
  std::memcpy(this->_pMappedMemory + bufferOffset, data.data(), data.size());
}

} // namespace AltheaEngine