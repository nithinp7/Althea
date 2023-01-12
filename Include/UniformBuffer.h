#pragma once

#include "Allocator.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

// TODO: Make CPU-copy of uniform buffer optional?

template <typename TUniforms> class UniformBuffer {
public:
  // Move-only semantics
  UniformBuffer() = default;
  UniformBuffer(UniformBuffer&& rhs) = default;
  UniformBuffer& operator=(UniformBuffer&& rhs) = default;

  UniformBuffer(const UniformBuffer& rhs) = delete;
  UniformBuffer& operator=(const UniformBuffer& rhs) = delete;

  UniformBuffer(const Application& app) 
      : _allocation(app.createUniformBuffer(sizeof(TUniforms))) {}

  UniformBuffer(const Application& app, const TUniforms& uniforms)
      : _allocation(app.createUniformBuffer(sizeof(TUniforms))) {
    updateUniforms(uniforms);
  }

  // TODO: TUniforms& getUniforms()
  // TODO: syncBuffer()

  void updateUniforms(const TUniforms& uniforms) {
    this->_uniforms = uniforms;

    void* dst = this->_allocation.mapMemory();
    memcpy(dst, &this->_uniforms, sizeof(TUniforms));
    this->_allocation.unmapMemory();
  }

  const TUniforms& getUniforms() const { return this->_uniforms; }

  VkBuffer getAllocation() const { return this->_allocation; }

  size_t getSize() const { return sizeof(TUniforms); }

private:
  TUniforms _uniforms;
  BufferAllocation _allocation;
};

} // namespace AltheaEngine