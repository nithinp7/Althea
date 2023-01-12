#pragma once

#include "Allocator.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;

template <typename TVertex> class VertexBuffer {
public:
  // Move-only semantics
  VertexBuffer() = default;
  VertexBuffer(VertexBuffer&& rhs) = default;
  VertexBuffer& operator=(VertexBuffer&& rhs) = default;

  VertexBuffer(const VertexBuffer& rhs) = delete;
  VertexBuffer& operator=(const VertexBuffer& rhs) = delete;

  // TODO: move vertex buffer creation code into this class?

  VertexBuffer(const Application& app, std::vector<TVertex>&& vertices)
      : _vertices(std::move(vertices)), 
        _allocation(app.createVertexBuffer(
            (const void*)this->_vertices.data(),
            sizeof(TVertex) * this->_vertices.size())) {}

  const std::vector<TVertex>& getVertices() const { return this->_vertices; }

  size_t getVertexCount() const { return this->_vertices.size(); }

  const BufferAllocation& getAllocation() const { return this->_allocation; }

private:
  std::vector<TVertex> _vertices;
  BufferAllocation _allocation;
};
} // namespace AltheaEngine