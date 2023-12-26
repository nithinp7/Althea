#pragma once

#include "Library.h"
#include "UniformBuffer.h"
#include "FrameContext.h"
#include "GlobalHeap.h"

#include <glm/glm.hpp>

#include <vector>

namespace AltheaEngine {

struct GlobalUniforms {
  glm::mat4 projection;
  glm::mat4 inverseProjection;
  glm::mat4 view;
  glm::mat4 prevView;
  glm::mat4 inverseView;
  glm::mat4 prevInverseView;
  int lightCount;
  float time;
  float exposure;
};

class Application;

class ALTHEA_API GlobalUniformsResource {
public:
  GlobalUniformsResource() = default;
  GlobalUniformsResource(const Application& app, GlobalHeap& heap);
  
  UniformBuffer<GlobalUniforms>& getCurrentUniformBuffer(const FrameContext& frame) {
    return this->_buffers[frame.frameRingBufferIndex];
  }

  const UniformBuffer<GlobalUniforms>& getCurrentUniformBuffer(const FrameContext& frame) const {
    return this->_buffers[frame.frameRingBufferIndex];
  }

  BufferHandle getCurrentBindlessHandle(const FrameContext& frame) const {
    return this->_indices[frame.frameRingBufferIndex];
  }

private:
  UniformBuffer<GlobalUniforms> _buffers[MAX_FRAMES_IN_FLIGHT];
  BufferHandle _indices[MAX_FRAMES_IN_FLIGHT];
};
} // namespace AltheaEngine