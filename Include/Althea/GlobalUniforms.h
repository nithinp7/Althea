#pragma once

#include "Library.h"
#include "UniformBuffer.h"
#include "FrameContext.h"
#include "GlobalHeap.h"

#include <glm/glm.hpp>

#include <cstdint>
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
  uint32_t lightBufferHandle;
  float time;
  float exposure;

  glm::vec2 mouseUV;
  uint32_t inputMask;
  uint32_t padding;
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

  UniformHandle getCurrentBindlessHandle(const FrameContext& frame) const {
    return this->_indices[frame.frameRingBufferIndex];
  }

private:
  UniformBuffer<GlobalUniforms> _buffers[MAX_FRAMES_IN_FLIGHT];
  UniformHandle _indices[MAX_FRAMES_IN_FLIGHT];
};
} // namespace AltheaEngine