#pragma once

#include "Allocator.h"
#include "BindlessHandle.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;
class GlobalHeap;

class ALTHEA_API IndexBuffer {
public:
  IndexBuffer() = default;
  IndexBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkBuffer srcBuffer,
      size_t bufferOffset,
      size_t bufferSize);
  IndexBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      std::vector<uint32_t>&& indices);
  IndexBuffer(
      Application& app,
      VkCommandBuffer commandBuffer,
      std::vector<uint32_t>&& indices);

  void registerToHeap(GlobalHeap& heap);
  BufferHandle getHandle() const { return _handle; }
  
  const std::vector<uint32_t>& getIndices() const { return this->_indices; }

  size_t getIndexCount() const { return this->_indices.size(); }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

  size_t getSize() const { return getIndexCount() * sizeof(uint32_t); }

private:
  std::vector<uint32_t> _indices;
  BufferAllocation _allocation;
  BufferHandle _handle;
};
} // namespace AltheaEngine