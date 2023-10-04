#pragma once

#include "DeferredRendering.h"
#include "DescriptorSet.h"
#include "FrameBuffer.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "ImageResource.h"
#include "ImageView.h"
#include "Library.h"
#include "RenderPass.h"
#include "ResourcesAssignment.h"
#include "Sampler.h"
#include "ComputePipeline.h"
#include "PerFrameResources.h"
#include "ReflectionBuffer.h"

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
      VkDescriptorSetLayout globalSetLayout,
      const GBufferResources& gBuffer);
  void captureReflection(
      const Application& app,
      VkCommandBuffer commandBuffer,
      VkDescriptorSet globalSet,
      const FrameContext& context);
  void convolveReflectionBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      const FrameContext& context);

  void bindTexture(ResourcesAssignment& assignment) const;

private:
  ReflectionBuffer _reflectionBuffer;
  
  std::unique_ptr<RenderPass> _pReflectionPass;
  std::unique_ptr<DescriptorSetAllocator> _pGBufferMaterialAllocator;
  std::unique_ptr<Material> _pGBufferMaterial;
  FrameBuffer _reflectionFrameBuffer;
};
} // namespace AltheaEngine