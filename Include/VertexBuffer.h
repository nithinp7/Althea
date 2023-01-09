#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;

class VertexBuffer {
public:
  VertexBuffer() = default;
  VertexBuffer(const VertexBuffer& rhs) = delete;
  VertexBuffer(VertexBuffer&& rhs);
  VertexBuffer& operator=(VertexBuffer&& rhs);
  VertexBuffer& operator=(const VertexBuffer& rhs) = delete;

  template <typename TVertex>
  VertexBuffer(const Application& app, const std::vector<TVertex>& vertices)
      : _device(app.getDevice()) {
    // TODO: move vertex buffer creation code into this class?
    app.createVertexBuffer(
        (const void*)vertices.data(),
        sizeof(TVertex) * vertices.size(),
        this->_buffer,
        this->_memory);
  }

  ~VertexBuffer();

  VkBuffer getBuffer() const { return this->_buffer; }

private:
  VkDevice _device = VK_NULL_HANDLE;
  VkBuffer _buffer;
  // TODO: better memory managment
  VkDeviceMemory _memory;
};
} // namespace AltheaEngine