#pragma once

#include "DynamicBuffer.h"
#include "Library.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <stdexcept>

namespace AltheaEngine {
class Application;
class GlobalHeap;

template <typename TVertex> class ALTHEA_API DynamicVertexBuffer {
public:
  DynamicVertexBuffer() = default;
  DynamicVertexBuffer(
      const Application& app,
      size_t vertexCount,
      bool bRetainCpuCopy = false)
      : _vertexCount(vertexCount),
        _buffer(
            app,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            vertexCount * sizeof(TVertex)) {
    if (bRetainCpuCopy) {
      _vertices.resize(vertexCount);
    }
  }

  void registerToHeap(GlobalHeap& heap) { _buffer.registerToHeap(heap); }

  void zeroBuffer(VkCommandBuffer commandBuffer, uint32_t ringBufferIndex) {
    _buffer.zeroBuffer(commandBuffer, ringBufferIndex);
  }

  void zeroAllBuffers(VkCommandBuffer commandBuffer) {
    _buffer.zeroAllBuffers(commandBuffer);
  }

  BufferHandle getCurrentBufferHandle(uint32_t ringBufferIndex) const {
    return _buffer.getCurrentBufferHandle(ringBufferIndex);
  }

  void
  updateVertices(uint32_t ringBufferIndex, gsl::span<const TVertex> vertices) {
    if (vertices.size() > this->_vertexCount) {
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

  // Note: bRetainCpuCopy needs to be true to use these functions
  TVertex& getVertex(uint32_t vertexIdx) { return _vertices[vertexIdx]; }
  const TVertex& getVertex(uint32_t vertexIdx) const {
    return _vertices[vertexIdx];
  }
  void setVertex(const TVertex& vert, uint32_t vertexIdx) {
    _vertices[vertexIdx] = vert;
  }
  const std::vector<TVertex>& getVertices() const { return _vertices; }
  void upload(uint32_t ringBufferIndex) {
    updateVertices(ringBufferIndex, gsl::span(_vertices.data(), _vertexCount));
  }

  const BufferAllocation& getAllocation() const {
    return this->_buffer.getAllocation();
  }

  void bind(uint32_t ringBufferIndex, VkCommandBuffer commandBuffer) const {
    VkBuffer vkBuffer = this->getBuffer();
    VkDeviceSize offset =
        sizeof(TVertex) * this->_vertexCount * ringBufferIndex;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkBuffer, &offset);
  }

  size_t getCurrentBufferOffset(uint32_t ringBufferIndex) const {
    return sizeof(TVertex) * this->_vertexCount * ringBufferIndex;
  }

private:
  size_t _vertexCount;
  DynamicBuffer _buffer;
  std::vector<TVertex> _vertices;
};
} // namespace AltheaEngine