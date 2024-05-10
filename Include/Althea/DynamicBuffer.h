#pragma once

#include "Allocator.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"
#include "GlobalHeap.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace AltheaEngine {
class Application;

/**
 * @brief Manages a double-buffered, per-frame dynamic buffer that is expected
 * to be updated every frame.
 */
class ALTHEA_API DynamicBuffer {
public:
  DynamicBuffer() = default;
  DynamicBuffer(DynamicBuffer&& rhs);
  DynamicBuffer& operator=(DynamicBuffer&& rhs);
  DynamicBuffer(const DynamicBuffer& rhs) = delete;
  DynamicBuffer& operator=(const DynamicBuffer& rhs) = delete;

  void zeroBuffer(VkCommandBuffer commandBuffer, uint32_t ringBufferIndex);
  void zeroAllBuffers(VkCommandBuffer commandBuffer);

  DynamicBuffer(
      const Application& app,
      VkBufferUsageFlags usage,
      size_t bufferSize,
      size_t offsetAlignment = 1);
  ~DynamicBuffer();

  void registerToHeap(GlobalHeap& heap);

  void updateData(uint32_t ringBufferIndex, gsl::span<const std::byte> data);

  size_t getSize() const { return this->_bufferSize; }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

  BufferHandle getCurrentBufferHandle(uint32_t ringBufferIndex) const {
    return this->_indices[ringBufferIndex];
  }

private:
  size_t _bufferSize;
  BufferAllocation _allocation;
  std::byte* _pMappedMemory = nullptr;

  BufferHandle _indices[MAX_FRAMES_IN_FLIGHT];
};
} // namespace AltheaEngine