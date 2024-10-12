#pragma once

#include "BindlessHandle.h"
#include "ComputePipeline.h"
#include "DeferredRendering.h"
#include "DescriptorSet.h"
#include "FrameBuffer.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "ImageResource.h"
#include "ImageView.h"
#include "Library.h"
#include "PerFrameResources.h"
#include "RenderPass.h"
#include "ResourcesAssignment.h"
#include "Sampler.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;
class GlobalHeap;

class ReflectionBuffer {
public:
  ReflectionBuffer() = default;
  ReflectionBuffer(const Application& app, VkCommandBuffer commandBuffer, GlobalHeap& heap);

  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToStorageImageWrite(
      VkCommandBuffer commandBuffer,
      VkPipelineStageFlags dstPipelineStageFlags);
  void bindTexture(ResourcesAssignment& assignment) const;

  const ImageView& getReflectionBufferTargetView() const {
    return this->_convolvedMips[0].view;
  }
  const Sampler& getReflectionBufferTargetSampler() const {
    return this->_mipSampler;
  }

  TextureHandle getHandle() const { return _reflectionBuffer.textureHandle; }

  void convolveReflectionBuffer(
    const Application& app,
    VkCommandBuffer commandBuffer,
    VkDescriptorSet heapSet,
    const FrameContext& context,
    VkImageLayout prevLayout,
    VkAccessFlags prevAccess,
    VkPipelineStageFlags srcStage);

private:
  ComputePipeline _convolutionPass;

  // Entire reflection buffer resource
  // The view contains all the mips together
  ImageResource _reflectionBuffer;

  struct ConvolvedMip {
    ImageView view;
    ImageHandle imgHandle;
    TextureHandle texHandle;
  };
  std::vector<ConvolvedMip> _convolvedMips;
  
  // The sampler to use when viewing a single mip of the
  // reflection buffer.
  Sampler _mipSampler;
};
} // namespace AltheaEngine