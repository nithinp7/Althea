#include "IndexBuffer.h"

#include "Application.h"

namespace AltheaEngine {
IndexBuffer::IndexBuffer(
    const Application& app,
    const std::vector<uint32_t>& indices)
    : _device(app.getDevice()) {
  app.createIndexBuffer(
      (const void*)indices.data(),
      sizeof(uint32_t) * indices.size(),
      this->_buffer,
      this->_memory);
}

IndexBuffer::IndexBuffer(IndexBuffer&& rhs)
    : _device(rhs._device), _buffer(rhs._buffer), _memory(rhs._memory) {
  rhs._device = VK_NULL_HANDLE;
  rhs._buffer = VK_NULL_HANDLE;
  rhs._memory = VK_NULL_HANDLE;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& rhs) {
  this->_device = rhs._device;
  this->_buffer = rhs._buffer;
  this->_memory = rhs._memory;

  rhs._device = VK_NULL_HANDLE;
  rhs._buffer = VK_NULL_HANDLE;
  rhs._memory = VK_NULL_HANDLE;

  return *this;
}

IndexBuffer::~IndexBuffer() {
  if (this->_device != VK_NULL_HANDLE) {
    vkDestroyBuffer(this->_device, this->_buffer, nullptr);
    vkFreeMemory(this->_device, this->_memory, nullptr);
  }
}
} // namespace AltheaEngine