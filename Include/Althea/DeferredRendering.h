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
#include "BindlessHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;
class GlobalHeap;

struct ALTHEA_API GBufferHandles {
  uint32_t positionHandle;
  uint32_t normalHandle;
  uint32_t albedoHandle;
  uint32_t metallicRoughnessOcclusionHandle;
};

class ALTHEA_API GBufferResources {
public:
  static void buildMaterial(
      DescriptorSetLayoutBuilder& builder,
      VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL);

  GBufferResources() = default;
  GBufferResources(const Application& app);
  void bindTextures(ResourcesAssignment& assignment) const;
  void transitionToAttachment(VkCommandBuffer commandBuffer);
  void transitionToTextures(VkCommandBuffer commandBuffer);

  void registerToHeap(GlobalHeap& heap);

  const std::vector<Attachment>& getAttachmentDescriptions() const {
    return this->_attachmentDescriptions;
  }

  const std::vector<VkImageView>& getAttachmentViews() const {
    return this->_attachmentViews;
  }

  GBufferHandles getHandles() const {
    return {
      this->_positionHandle.index,
      this->_normalHandle.index,
      this->_albedoHandle.index,
      this->_metallicRoughnessOcclusionHandle.index
    };
  }

private:
  ImageResource _position{};
  ImageResource _normal{};
  ImageResource _albedo{};
  ImageResource _metallicRoughnessOcclusion{};

  ImageHandle _positionHandle{};
  ImageHandle _normalHandle{};
  ImageHandle _albedoHandle{};
  ImageHandle _metallicRoughnessOcclusionHandle{};

  std::vector<Attachment> _attachmentDescriptions;
  std::vector<VkImageView> _attachmentViews;
};
} // namespace AltheaEngine
