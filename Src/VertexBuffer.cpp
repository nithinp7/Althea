#include "VertexBuffer.h"

namespace AltheaEngine {
VertexBuffer::VertexBuffer(VertexBuffer&& rhs)
    : _device(rhs._device), _buffer(rhs._buffer), _memory(rhs._memory) {
  rhs._device = VK_NULL_HANDLE;
  rhs._buffer = VK_NULL_HANDLE;
  rhs._memory = VK_NULL_HANDLE;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& rhs) {
  this->_device = rhs._device;
  this->_buffer = rhs._buffer;
  this->_memory = rhs._memory;

  rhs._device = VK_NULL_HANDLE;
  rhs._buffer = VK_NULL_HANDLE;
  rhs._memory = VK_NULL_HANDLE;

  return *this;
}

VertexBuffer::~VertexBuffer() {
  if (this->_device != VK_NULL_HANDLE) {
    vkDestroyBuffer(this->_device, this->_buffer, nullptr);
    vkFreeMemory(this->_device, this->_memory, nullptr);
  }
}
} // namespace AltheaEngine