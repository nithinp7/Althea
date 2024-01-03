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
  ReflectionBuffer(const Application& app, VkCommandBuffer commandBuffer);

  ReflectionBuffer(ReflectionBuffer&& other) {
    this->_pConvolutionPass = std::move(other._pConvolutionPass);
    this->_convolutionMaterials = std::move(other._convolutionMaterials);
    this->_pConvolutionMaterialAllocator =
        std::move(other._pConvolutionMaterialAllocator);

    this->_reflectionBuffer = std::move(other._reflectionBuffer);
    this->_reflectionBufferHandle = other._reflectionBufferHandle;

    this->_mipViews = std::move(other._mipViews);

    this->_mipSampler = std::move(other._mipSampler);
  }

  // TODO: This descriptor set architecture is becoming a pain in the ass
  ReflectionBuffer& operator=(ReflectionBuffer&& other) {
    this->_convolutionMaterials.clear();
    this->_pConvolutionMaterialAllocator.reset();

    this->_pConvolutionPass = std::move(other._pConvolutionPass);
    this->_convolutionMaterials = std::move(other._convolutionMaterials);
    this->_pConvolutionMaterialAllocator =
        std::move(other._pConvolutionMaterialAllocator);

    this->_reflectionBuffer = std::move(other._reflectionBuffer);
    this->_reflectionBufferHandle = other._reflectionBufferHandle;

    this->_mipViews = std::move(other._mipViews);

    this->_mipSampler = std::move(other._mipSampler);

    return *this;
  }

  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToStorageImageWrite(
      VkCommandBuffer commandBuffer,
      VkPipelineStageFlags dstPipelineStageFlags);
  void convolveReflectionBuffer(
      const Application& app,
      VkCommandBuffer commandBuffer,
      const FrameContext& context,
      VkImageLayout prevLayout,
      VkAccessFlags prevAccess,
      VkPipelineStageFlags srcStage);
  void bindTexture(ResourcesAssignment& assignment) const;

  const ImageView& getReflectionBufferTargetView() const {
    return this->_mipViews[0];
  }
  const Sampler& getReflectionBufferTargetSampler() const {
    return this->_mipSampler;
  }

  void registerToHeap(GlobalHeap& heap);

  ImageHandle getHandle() const { return this->_reflectionBufferHandle; }

private:
  std::unique_ptr<ComputePipeline> _pConvolutionPass;
  std::unique_ptr<DescriptorSetAllocator> _pConvolutionMaterialAllocator;
  std::vector<Material> _convolutionMaterials;

  // Entire reflection buffer resource
  // The view contains all the mips together
  ImageResource _reflectionBuffer;
  ImageHandle _reflectionBufferHandle;

  // Individual mip views of the reflection buffer mips
  std::vector<ImageView> _mipViews;
  // The sampler to use when viewing a single mip of the
  // reflection buffer.
  Sampler _mipSampler;
};
} // namespace AltheaEngine