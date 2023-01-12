#include "IndexBuffer.h"

#include "Application.h"

namespace AltheaEngine {
IndexBuffer::IndexBuffer(
    const Application& app,
    std::vector<uint32_t>&& indices)
    : _indices(std::move(indices)),
      _allocation(app.createIndexBuffer(
          (const void*)this->_indices.data(),
          sizeof(uint32_t) * this->_indices.size())) {}
} // namespace AltheaEngine