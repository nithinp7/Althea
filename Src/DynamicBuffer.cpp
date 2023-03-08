#include "DynamicBuffer.h"

#include "Application.h"

namespace AltheaEngine {

DynamicBuffer::DynamicBuffer(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkBufferUsageFlags usage,
    size_t bufferSize)
    : _bufferSize(bufferSize) {
  // Create double buffered persistently mapped region
  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  this->_allocation = BufferUtilities::createBuffer(
      app,
      commandBuffer,
      bufferSize * app.getMaxFramesInFlight(),
      usage,
      deviceAllocInfo);

  this->_pMappedMemory =
      reinterpret_cast<std::byte*>(this->_allocation.mapMemory());
}

DynamicBuffer::~DynamicBuffer() { this->_allocation.unmapMemory(); }

void DynamicBuffer::updateData(
    const Application& app,
    VkCommandBuffer commandBuffer,
    gsl::span<const std::byte> data) {
  if (data.size() != this->_bufferSize) {
    throw std::runtime_error("Attempting to update DynamicBuffer with "
                             "incorrect buffer size.");
  }

  size_t bufferOffset =
      app.getCurrentFrameRingBufferIndex() * this->_bufferSize;
  std::memcpy(
      this->_pMappedMemory + bufferOffset,
      data.data(),
      this->_bufferSize);
}

} // namespace AltheaEngine