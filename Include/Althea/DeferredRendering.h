#pragma once

#include "Attachment.h"
#include "BindlessHandle.h"
#include "DescriptorSet.h"
#include "FrameBuffer.h"
#include "Framebuffer.h"
#include "GraphicsPipeline.h"
#include "ImageResource.h"
#include "IntrusivePtr.h"
#include "Library.h"
#include "Material.h"
#include "RenderPass.h"
#include "ResourcesAssignment.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

namespace AltheaEngine {
class Application;
class GlobalHeap;

struct ALTHEA_API GBufferHandles {
  uint32_t depthAHandle;
  uint32_t depthBHandle;
  uint32_t normalHandle;
  uint32_t albedoHandle;
  uint32_t metallicRoughnessOcclusionHandle;
  uint32_t stencilAHandle;
  uint32_t stencilBHandle;
};

class SubpassBuilder;

class ALTHEA_API GBufferResources {
public:
  static void setupAttachments(SubpassBuilder& subpassBuilder);
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

  const std::vector<VkImageView>& getAttachmentViewsA() const {
    return this->_attachmentViewsA;
  }

  const std::vector<VkImageView>& getAttachmentViewsB() const {
    return this->_attachmentViewsB;
  }

  ImageResource& getDepthA() { return _depthA; }
  ImageResource& getDepthB() { return _depthB; }
  VkImageView getDepthViewA() const { return _depthA.view; }
  VkImageView getDepthViewB() const { return _depthB.view; }

  GBufferHandles getHandles() const {
    return {
        this->_depthAHandle.index,
        this->_depthBHandle.index,
        this->_normalHandle.index,
        this->_albedoHandle.index,
        this->_metallicRoughnessOcclusionHandle.index,
        this->_stencilAHandle.index,
        this->_stencilBHandle.index};
  }

private:
  ImageResource _depthA{};
  ImageResource _depthB{};
  ImageResource _normal{};
  ImageResource _albedo{};
  ImageResource _metallicRoughnessOcclusion{};

  ImageView _stencilAView{};
  ImageView _stencilBView{};
  TextureHandle _stencilAHandle{};
  TextureHandle _stencilBHandle{};

  TextureHandle _depthAHandle{};
  TextureHandle _depthBHandle{};
  TextureHandle _normalHandle{};
  TextureHandle _albedoHandle{};
  TextureHandle _metallicRoughnessOcclusionHandle{};

  std::vector<Attachment> _attachmentDescriptions;
  std::vector<VkImageView> _attachmentViewsA;
  std::vector<VkImageView> _attachmentViewsB;
};

class IGBufferSubpass : public virtual RefCounted {
public:
  IGBufferSubpass() = default;
  virtual ~IGBufferSubpass() = default;

  virtual void
  registerGBufferSubpass(GraphicsPipelineBuilder& builder) const = 0;
  virtual void beginGBufferSubpass(
      const DrawContext& context,
      BufferHandle globalResourcesHandle,
      UniformHandle globalUniformsHandle) = 0;
};

class SceneToGBufferPassBuilder {
  friend class SceneToGBufferPass;

public:
  void registerSubpass(const IntrusivePtr<IGBufferSubpass>& subpass) {
    _subpasses.push_back(subpass);
  }

  void disableDepthDoubleBuffering() { _doubleBufferDepth = false; }

private:
  std::vector<IntrusivePtr<IGBufferSubpass>> _subpasses;
  bool _doubleBufferDepth = true;
};

class SceneToGBufferPass {
public:
  SceneToGBufferPass() = default;
  SceneToGBufferPass(
      const Application& app,
      GBufferResources& gBuffer,
      VkDescriptorSetLayout heap,
      SceneToGBufferPassBuilder&& builder);

  void begin(
      const Application& app,
      VkCommandBuffer commandBuffer,
      const FrameContext& frame,
      VkDescriptorSet heapSet,
      BufferHandle globalResourcesHandle,
      UniformHandle globalUniformsHandle);

  void tryRecompileShaders(Application& app) { _pass.tryRecompile(app); }

private:
  SceneToGBufferPassBuilder _builder;

  RenderPass _pass;
  bool _phase = true;
  FrameBuffer _fbA;
  FrameBuffer _fbB;
};
} // namespace AltheaEngine
