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

class ReflectionBuffer {
public:
  ReflectionBuffer() = default;
  ReflectionBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer);
  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void convolveReflectionBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      const FrameContext& context);
  void bindTexture(ResourcesAssignment& assignment) const;

  const ImageView& getReflectionBufferTargetView() const { return this->_mipViews[0]; }
  const Sampler& getReflectionBufferTargetSampler() const { return this->_mipSampler; }
private:
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