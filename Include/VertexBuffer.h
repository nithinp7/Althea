#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;

template <typename TVertex> class VertexBuffer {
public:
  VertexBuffer() = default;

  VertexBuffer(VertexBuffer&& rhs)
      : _vertices(std::move(rhs._vertices)),
        _device(rhs._device),
        _buffer(rhs._buffer),
        _memory(rhs._memory) {
    rhs._device = VK_NULL_HANDLE;
    rhs._buffer = VK_NULL_HANDLE;
    rhs._memory = VK_NULL_HANDLE;
  }

  VertexBuffer& operator=(VertexBuffer&& rhs) {
    this->_vertices = std::move(rhs._vertices);
    this->_device = rhs._device;
    this->_buffer = rhs._buffer;
    this->_memory = rhs._memory;

    rhs._device = VK_NULL_HANDLE;
    rhs._buffer = VK_NULL_HANDLE;
    rhs._memory = VK_NULL_HANDLE;

    return *this;
  }

  VertexBuffer(const VertexBuffer& rhs) = delete;
  VertexBuffer& operator=(const VertexBuffer& rhs) = delete;

  VertexBuffer(const Application& app, std::vector<TVertex>&& vertices)
      : _vertices(std::move(vertices)), _device(app.getDevice()) {
    // TODO: move vertex buffer creation code into this class?
    app.createVertexBuffer(
        (const void*)this->_vertices.data(),
        sizeof(TVertex) * this->_vertices.size(),
        this->_buffer,
        this->_memory);
  }

  ~VertexBuffer() {
    if (this->_device != VK_NULL_HANDLE) {
      vkDestroyBuffer(this->_device, this->_buffer, nullptr);
      vkFreeMemory(this->_device, this->_memory, nullptr);
    }
  }

  const std::vector<TVertex>& getVertices() const { return this->_vertices; }

  size_t getVertexCount() const { return this->_vertices.size(); }

  VkBuffer getBuffer() const { return this->_buffer; }

private:
  std::vector<TVertex> _vertices;

  VkDevice _device = VK_NULL_HANDLE;
  VkBuffer _buffer;
  // TODO: better memory managment
  VkDeviceMemory _memory;
};
} // namespace AltheaEngine