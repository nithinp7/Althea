#pragma once

#include "Library.h"

#include "FrameContext.h"
#include "UniformBuffer.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;

// TODO: Make CPU-copy of uniform buffer optional.

/**
 * @brief A double-buffered UBO that is intended to be only used for
 * transient, per-frame data. Uniform updates will only be written to the
 * underlying UBO corresponding to the current frame.
 */
template <typename TUniforms> class ALTHEA_API TransientUniforms {
public:
  TransientUniforms() = default;

  TransientUniforms(const Application& app) {
    for (uint32_t i = 0; i < app.getMaxFramesInFlight(); ++i) {
      this->_uniformBuffers.emplace_back(app);
    }
  }

  TransientUniforms(const Application& app, const TUniforms& uniforms) {
    for (uint32_t i = 0; i < app.getMaxFramesInFlight(); ++i) {
      this->_uniformBuffers.emplace_back(app, uniforms);
    }
  }

  void updateUniforms(const TUniforms& uniforms, const FrameContext& frame) {
    this->_uniformBuffers[frame.frameRingBufferIndex].updateUniforms(uniforms);
  }

  const std::vector<UniformBuffer<TUniforms>>& getUniformBuffers() const {
    return this->_uniformBuffers;
  }

  const UniformBuffer<TUniforms>&
  getCurrentUniformBuffer(const FrameContext& frame) const {
    return this->_uniformBuffers[frame.frameRingBufferIndex];
  }

  size_t getSize() const { return sizeof(TUniforms); }

private:
  std::vector<UniformBuffer<TUniforms>> _uniformBuffers;
};

} // namespace AltheaEngine