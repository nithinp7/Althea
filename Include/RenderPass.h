#pragma once

#include "DescriptorSet.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "PerFrameResources.h"
#include "DrawContext.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <vector>
#include <memory>

namespace AltheaEngine {
class Application;

enum class AttachmentType {
  Color,
  Depth
  // Stencil
};

struct Attachment {
  /**
   * @brief The type of attachment to create.
   */
  AttachmentType type;

  /**
   * @brief The image format of this attachment.
   */
  VkFormat format;

  /**
   * @brief The clear color to use when loading this attachment.
   */
  VkClearValue clearValue;

  /**
   * @brief The image required to construct the frame buffer for
   * this render pass.
   *
   * If this attachment will be used for presentation, leave this empty.
   * In that case, a frame buffer will be created for each swapchain image.
   */
  std::optional<VkImageView> frameBufferImageView = std::nullopt;

  /**
   * @brief This attachment is only used inside this render pass and the results
   * will not be used by subsequent uses of the image (e.g., depth attachment).
   */
  bool internalUsageOnly = false;
};

// TODO: Generalize to non-graphics pipelines
struct SubpassBuilder {
  std::vector<uint32_t> colorAttachments;
  std::optional<uint32_t> depthAttachment;

  DescriptorSetLayoutBuilder subpassResourcesBuilder;

  GraphicsPipelineBuilder pipelineBuilder;
};

class Subpass {
public:
  Subpass(
      const Application& app,
      VkRenderPass renderPass,
      const std::shared_ptr<PerFrameResources>& pGlobalResources,
      const std::optional<PerFrameResources>& renderPassResources,
      uint32_t subpassIndex,
      const SubpassBuilder& builder);
  Subpass(Subpass&& rhs) = default;

  const GraphicsPipeline& getPipeline() const { return this->_pipeline; }

  GraphicsPipeline& getPipeline() { return this->_pipeline; }

  std::optional<PerFrameResources>& getSubpassResources() {
    return this->_subpassResources;
  }

  const std::optional<PerFrameResources>& getSubpassResources() const {
    return this->_subpassResources;
  }

private:
  std::optional<PerFrameResources> _subpassResources;

  GraphicsPipeline _pipeline;
};

class ActiveRenderPass;
class RenderPass {
public:
  RenderPass(
      const Application& app,
      const std::shared_ptr<PerFrameResources>& pGlobalResources,
      const DescriptorSetLayoutBuilder& renderPassResourcesLayoutBuilder,
      std::vector<Attachment>&& attachments,
      std::vector<SubpassBuilder>&& subpasses);
  ~RenderPass();

  ActiveRenderPass begin(
      const Application& app,
      const VkCommandBuffer& commandBuffer,
      const FrameContext& frame);
  
  std::optional<PerFrameResources>& getRenderPassResources() {
    return this->_renderPassResources;
  }

  const std::optional<PerFrameResources>& getRenderPassResources() const {
    return this->_renderPassResources;
  }

  const std::vector<Subpass>& getSubpasses() const { return this->_subpasses; }
  std::vector<Subpass>& getSubpasses() { return this->_subpasses; }

private:
  friend class ActiveRenderPass;

  void _createFrameBuffer(
      const VkExtent2D& extent,
      const std::optional<VkImageView>& swapChainImageView);

  std::shared_ptr<PerFrameResources> _pGlobalResources;
  std::optional<PerFrameResources> _renderPassResources;

  std::vector<Attachment> _attachments;
  std::vector<Subpass> _subpasses;

  VkRenderPass _renderPass;

  std::vector<VkFramebuffer> _frameBuffers;

  VkDevice _device;

  bool _firstAttachmentFromSwapChain = false;
};

class ActiveRenderPass {
public:
  ActiveRenderPass(
      const RenderPass& renderPass,
      const VkCommandBuffer& commandBuffer,
      const PerFrameResources* pGlobalResources,
      const FrameContext& frame,
      const VkExtent2D& extent);
  ~ActiveRenderPass();

  ActiveRenderPass& nextSubpass();

  // TODO: create IDrawable interface instead?
  template <typename TDrawable>
  ActiveRenderPass& draw(const TDrawable& object) {
    const DrawContext& context = this->_drawContext;
    object.draw(context);

    return *this;
  }

private:
  uint32_t _currentSubpass = 0;

  const RenderPass& _renderPass;
  const VkCommandBuffer& _commandBuffer;
  const FrameContext& _frame;

  std::vector<VkClearValue> _clearValues;

  DrawContext _drawContext;
};
} // namespace AltheaEngine