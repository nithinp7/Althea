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

  IndexBuffer(const Application& app, std::vector<uint32_t>&& indices);
  ~IndexBuffer();

  const std::vector<uint32_t>& getIndices() const { return this->_indices; }

  size_t getIndexCount() const { return this->_indices.size(); }

  VkBuffer getBuffer() const { return this->_buffer; }

private:
  std::vector<uint32_t> _indices;

  VkDevice _device = VK_NULL_HANDLE;
  VkBuffer _buffer;

  VkDeviceMemory _memory;
};
} // namespace AltheaEngine