#pragma once

#include "Allocator.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;

class IndexBuffer {
public:
  // Move-only semantics
  IndexBuffer() = default;
  IndexBuffer(IndexBuffer&& rhs) = default;
  IndexBuffer& operator=(IndexBuffer&& rhs) = default;

  IndexBuffer(const IndexBuffer& rhs) = delete;
  IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

  IndexBuffer(const Application& app, std::vector<uint32_t>&& indices);

  const std::vector<uint32_t>& getIndices() const { return this->_indices; }

  size_t getIndexCount() const { return this->_indices.size(); }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

private:
  std::vector<uint32_t> _indices;
  BufferAllocation _allocation;
};
} // namespace AltheaEngine