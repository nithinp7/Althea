#pragma once

#include "DynamicBuffer.h"
#include "Library.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <stdexcept>

namespace AltheaEngine {
class Application;

template <typename TVertex> class ALTHEA_API DynamicVertexBuffer {
public:
  DynamicVertexBuffer() = default;
  DynamicVertexBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      size_t vertexCount)
      : _vertexCount(vertexCount),
        _buffer(
            app,
            commandBuffer,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            vertexCount * sizeof(TVertex)) {}

  void
  updateVertices(uint32_t ringBufferIndex, gsl::span<const TVertex> vertices) {
    if (vertices.size() != this->_vertexCount) {
      throw std::runtime_error("Attempting to update DynamicVertexBuffer with "
                               "incorrect number of vertices.");
    }

    gsl::span<const std::byte> bufferView(
        reinterpret_cast<const std::byte*>(vertices.data()),
        sizeof(TVertex) * this->_vertexCount);

    this->_buffer.updateData(ringBufferIndex, bufferView);
  }

  VkBuffer getBuffer() const {
    return this->_buffer.getAllocation().getBuffer();
  }

  size_t getVertexCount() const { return this->_vertexCount; }

  const BufferAllocation& getAllocation() const {
    return this->_buffer.getAllocation();
  }

  void bind(uint32_t ringBufferIndex, VkCommandBuffer commandBuffer) const {
    VkBuffer vkBuffer = this->getBuffer();
    VkDeviceSize offset =
        sizeof(TVertex) * this->_vertexCount * ringBufferIndex;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkBuffer, &offset);
  }

private:
  size_t _vertexCount;
  DynamicBuffer _buffer;
};
} // namespace AltheaEngine