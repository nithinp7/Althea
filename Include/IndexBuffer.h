#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;

class IndexBuffer {
public:
  IndexBuffer() = default;
  IndexBuffer(const IndexBuffer& rhs) = delete;
  IndexBuffer(IndexBuffer&& rhs);
  IndexBuffer& operator=(IndexBuffer&& rhs);
  IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

  IndexBuffer(const Application& app, const std::vector<uint32_t>& indices);
  ~IndexBuffer();

  VkBuffer getBuffer() const { return this->_buffer; }

private:
  VkDevice _device = VK_NULL_HANDLE;
  VkBuffer _buffer;

  VkDeviceMemory _memory;
};
} // namespace AltheaEngine