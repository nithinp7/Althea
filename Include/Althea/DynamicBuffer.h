#pragma once

#include "Allocator.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

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

  DynamicBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkBufferUsageFlags usage,
      size_t bufferSize);
  ~DynamicBuffer();

  void updateData(uint32_t ringBufferIndex, gsl::span<const std::byte> data);

  size_t getSize() const { return this->_bufferSize; }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

private:
  size_t _bufferSize;
  BufferAllocation _allocation;
  std::byte* _pMappedMemory = nullptr;
};
} // namespace AltheaEngine