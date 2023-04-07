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
  void transitionToAttachment(VkCommandBuffer commandBuffer);
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
  std::unique_ptr<RenderPass> _pReflectionPass;
  std::unique_ptr<DescriptorSetAllocator> _pGBufferMaterialAllocator;
  std::unique_ptr<Material> _pGBufferMaterial;
  FrameBuffer _reflectionFrameBuffer;

  std::unique_ptr<ComputePipeline> _pConvolutionPass;
  std::unique_ptr<DescriptorSetAllocator> _pConvolutionMaterialAllocator;
  std::vector<Material> _convolutionMaterials;

  // Entire reflection buffer resource
  // The view contains all the mips together
  ImageResource _reflectionBuffer;

  // Individual mip views of the reflection buffer mips
  std::vector<ImageView> _mipViews;
  // The sampler to use when viewing a single mip of the
  // reflection buffer.
  Sampler _mipSampler;
};
} // namespace AltheaEngine