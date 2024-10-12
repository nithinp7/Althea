#pragma once

#include "BindlessHandle.h"
#include "ComputePipeline.h"
#include "DeferredRendering.h"
#include "FrameBuffer.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "ImageResource.h"
#include "ImageView.h"
#include "Library.h"
#include "PerFrameResources.h"
#include "ReflectionBuffer.h"
#include "RenderPass.h"
#include "ResourcesAssignment.h"
#include "Sampler.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

class ALTHEA_API ScreenSpaceReflection {
public:
  ScreenSpaceReflection() = default;
  ScreenSpaceReflection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      GlobalHeap& heap);

  void captureReflection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkDescriptorSet heapSet,
      const FrameContext& context,
      UniformHandle globalUniforms,
      BufferHandle globalResources);
  void convolveReflectionBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkDescriptorSet heapSet,
      const FrameContext& context);

  void bindTexture(ResourcesAssignment& assignment) const;

  void tryRecompileShaders(Application& app) {
    _reflectionPass.tryRecompile(app);
  }

  const ReflectionBuffer& getReflectionBuffer() const {
    return this->_reflectionBuffer;
  }

  ReflectionBuffer& getReflectionBuffer() { return this->_reflectionBuffer; }

private:
  ReflectionBuffer _reflectionBuffer;

  RenderPass _reflectionPass;
  FrameBuffer _reflectionFrameBuffer;
};
} // namespace AltheaEngine