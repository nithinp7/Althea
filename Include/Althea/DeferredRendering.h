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
