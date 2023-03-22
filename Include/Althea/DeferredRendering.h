#pragma once

#include "Attachment.h"
#include "DescriptorSet.h"
#include "FrameBuffer.h"
#include "GraphicsPipeline.h"
#include "ImageResource.h"
#include "Library.h"
#include "Material.h"
#include "ResourcesAssignment.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;

class ALTHEA_API GBufferResources {
public:
  static void buildMaterial(DescriptorSetLayoutBuilder& builder);

  GBufferResources() = default;
  GBufferResources(const Application& app);
  void bindTextures(ResourcesAssignment& assignment);
  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToTextures(VkCommandBuffer commandBuffer);

  VkImage getPositionImage() const { return this->_position.image; }

  VkImageView getPositionView() const { return this->_position.view; }

  VkSampler getPositionSampler() const { return this->_position.sampler; }

  VkImage getNormalImage() const { return this->_normal.image; }

  VkImageView getNormalView() const { return this->_normal.view; }

  VkSampler getNormalSampler() const { return this->_normal.sampler; }

  VkImage getAlbedoImage() const { return this->_albedo.image; }

  VkImageView getAlbedoView() const { return this->_albedo.view; }

  VkSampler getAlbedoSampler() const { return this->_albedo.sampler; }

  VkImage getMetallicRoughnessOcclusionImage() const {
    return this->_metallicRoughnessOcclusion.image;
  }

  VkImageView getMetallicRoughnessOcclusionView() const {
    return this->_metallicRoughnessOcclusion.view;
  }

  VkSampler getMetallicRoughnessOcclusionSampler() const {
    return this->_metallicRoughnessOcclusion.sampler;
  }

  const std::vector<Attachment>& getAttachmentDescriptions() const {
    return this->_attachmentDescriptions;
  }

  const std::vector<VkImageView>& getAttachmentViews() const {
    return this->_attachmentViews;
  }

private:
  ImageResource _position{};
  ImageResource _normal{};
  ImageResource _albedo{};
  ImageResource _metallicRoughnessOcclusion{};

  std::vector<Attachment> _attachmentDescriptions;
  std::vector<VkImageView> _attachmentViews;
};

namespace DeferredRendering {
void buildForwardPipeline(GraphicsPipelineBuilder& pipeline);
void buildLayout(DescriptorSetLayoutBuilder& layoutBuilder);
} // namespace DeferredRendering
} // namespace AltheaEngine
