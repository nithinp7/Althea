#pragma once

#include "Allocator.h"
#include "BufferUtilities.h"
#include "Library.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

// TODO: Make CPU-copy of uniform buffer optional?

template <typename TUniforms> class ALTHEA_API UniformBuffer {
public:
  UniformBuffer() = default;

  UniformBuffer(const Application& app) {
    this->_createUniformBuffer(app);
  }

  UniformBuffer(
      const Application& app,
      const TUniforms& uniforms) {
    this->_createUniformBuffer(app);
    this->updateUniforms(uniforms);
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

  const BufferAllocation& getAllocation() const { return this->_allocation; }

  size_t getSize() const { return sizeof(TUniforms); }

private:
  void
  _createUniformBuffer(const Application& app) {
    // TODO: This assumes that the uniform buffer will be _often_ rewritten
    // and perhaps in a random pattern. We should prefer a different type of
    // memory if the uniform buffer will mostly be persistent.
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    this->_allocation = BufferUtilities::createBuffer(
        app,
        sizeof(TUniforms),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        allocInfo);
  }

  TUniforms _uniforms;
  BufferAllocation _allocation;
};

} // namespace AltheaEngine