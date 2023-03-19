#pragma once

#include "DescriptorSet.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "Library.h"
#include "PerFrameResources.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace AltheaEngine {
class Application;

enum ALTHEA_API AttachmentTypeFlags {
  ATTACHMENT_FLAG_COLOR = 1,
  ATTACHMENT_FLAG_CUBEMAP = 2,
  ATTACHMENT_FLAG_DEPTH = 4,
  // Stencil
};

struct ALTHEA_API Attachment {
  /**
   * @brief The type of attachment to create.
   */
  int flags;

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
struct ALTHEA_API SubpassBuilder {
  std::vector<uint32_t> colorAttachments;
  std::optional<uint32_t> depthAttachment;

  GraphicsPipelineBuilder pipelineBuilder;
};

class ALTHEA_API Subpass {
public:
  Subpass(
      const Application& app,
      const VkExtent2D& extent,
      VkRenderPass renderPass,
      uint32_t subpassIndex,
      SubpassBuilder&& builder);
  Subpass(Subpass&& rhs) = default;

  const GraphicsPipeline& getPipeline() const { return this->_pipeline; }

  GraphicsPipeline& getPipeline() { return this->_pipeline; }

private:
  GraphicsPipeline _pipeline;
};

class ActiveRenderPass;
class ALTHEA_API RenderPass {
public:
  RenderPass(
      const Application& app,
      const VkExtent2D& extent,
      std::vector<Attachment>&& attachments,
      std::vector<SubpassBuilder>&& subpasses);
  ~RenderPass();

  ActiveRenderPass begin(
      const Application& app,
      const VkCommandBuffer& commandBuffer,
      const FrameContext& frame);

  const std::vector<Subpass>& getSubpasses() const { return this->_subpasses; }
  std::vector<Subpass>& getSubpasses() { return this->_subpasses; }

private:
  friend class ActiveRenderPass;

  void _createFrameBuffer(const std::optional<VkImageView>& swapChainImageView);

  std::vector<Attachment> _attachments;
  std::vector<Subpass> _subpasses;

  VkRenderPass _renderPass;

  std::vector<VkFramebuffer> _frameBuffers;

  VkDevice _device;
  // TODO: Seems weird baking the extent into the render pass...
  VkExtent2D _extent;

  bool _firstAttachmentFromSwapChain = false;
};

class ALTHEA_API ActiveRenderPass {
public:
  ActiveRenderPass(
      const RenderPass& renderPass,
      const VkCommandBuffer& commandBuffer,
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

  ActiveRenderPass&
  setGlobalDescriptorSets(gsl::span<const VkDescriptorSet> sets);

  const DrawContext& getDrawContext() const { return this->_drawContext; }

private:
  uint32_t _currentSubpass = 0;

  const RenderPass& _renderPass;
  const VkCommandBuffer& _commandBuffer;
  const FrameContext& _frame;

  std::vector<VkClearValue> _clearValues;

  DrawContext _drawContext;
};
} // namespace AltheaEngine