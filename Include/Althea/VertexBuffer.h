#pragma once

#include "Allocator.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;

template <typename TVertex> class ALTHEA_API VertexBuffer {
public:
  VertexBuffer() = default;

  VertexBuffer(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      std::vector<TVertex>&& vertices)
      : _vertices(std::move(vertices)) {
    gsl::span<const std::byte> verticesView(
        reinterpret_cast<const std::byte*>(this->_vertices.data()),
        sizeof(TVertex) * this->_vertices.size());

    VkBuffer stagingBuffer =
        commandBuffer.createStagingBuffer(app, verticesView);

    VmaAllocationCreateInfo deviceAllocInfo{};
    deviceAllocInfo.flags = 0;
    deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    this->_allocation = BufferUtilities::createBuffer(
        app,
        commandBuffer,
        verticesView.size(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        deviceAllocInfo);

    BufferUtilities::copyBuffer(
        commandBuffer,
        stagingBuffer,
        0,
        this->_allocation.getBuffer(),
        0,
        verticesView.size());
  }

  const std::vector<TVertex>& getVertices() const { return this->_vertices; }

  size_t getVertexCount() const { return this->_vertices.size(); }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

private:
  std::vector<TVertex> _vertices;
  BufferAllocation _allocation;
};
} // namespace AltheaEngine