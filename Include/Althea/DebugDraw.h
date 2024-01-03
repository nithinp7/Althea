#pragma once

#include "DeferredRendering.h"
#include "Library.h"
#include "RenderPass.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

struct ALTHEA_API DebugLineVertex {
  alignas(16) glm::vec3 position;
  alignas(4) uint32_t color;
};

class ALTHEA_API DebugDraw {
public:
  DebugDraw(
      const Application& app,
      const GBufferResources& gBuffer,
      VkDescriptorSetLayout debugDescriptorSetLayout,
      uint32_t debugBufferBinding);

  // StructuredBuffer<DebugLineVertex>& getBuffer(uint32_t bufferIdx) {
  //   return this->_buffers.getBuffer(bufferIdx);
  // }

  void draw(
      VkCommandBuffer commandBuffer,
      uint32_t bufferIdx,
      uint32_t primOffs,
      uint32_t primCount);

private:
  RenderPass _renderPass;
};
} // namespace AltheaEngine