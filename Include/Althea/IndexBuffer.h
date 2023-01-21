#pragma once

#include "Allocator.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;

class ALTHEA_API IndexBuffer {
public:
  IndexBuffer() = default;

  IndexBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      std::vector<uint32_t>&& indices);

  const std::vector<uint32_t>& getIndices() const { return this->_indices; }

  size_t getIndexCount() const { return this->_indices.size(); }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

private:
  std::vector<uint32_t> _indices;
  BufferAllocation _allocation;
};
} // namespace AltheaEngine