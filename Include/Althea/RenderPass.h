#pragma once

#include "Attachment.h"
#include "DescriptorSet.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "GraphicsPipeline.h"
#include "Library.h"
#include "PerFrameResources.h"
#include "UniqueVkHandle.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace AltheaEngine {
class Application;

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
  RenderPass() = default;

  RenderPass(
      const Application& app,
      const VkExtent2D& extent,
      std::vector<Attachment>&& attachments,
      std::vector<SubpassBuilder>&& subpasses,
      bool syncExternalBeforePass = true);

  ActiveRenderPass begin(
      const Application& app,
      const VkCommandBuffer& commandBuffer,
      const FrameContext& frame,
      VkFramebuffer frameBuffer) const;

  operator VkRenderPass() const { return this->_renderPass; }

  void tryRecompile(Application& app);

  const std::vector<Subpass>& getSubpasses() const { return this->_subpasses; }
  std::vector<Subpass>& getSubpasses() { return this->_subpasses; }

private:
  friend class ActiveRenderPass;

  struct RenderPassDeleter {
    void operator()(VkDevice device, VkRenderPass renderPass) {
      vkDestroyRenderPass(device, renderPass, nullptr);
    }
  };

  std::vector<Attachment> _attachments;
  std::vector<Subpass> _subpasses;

  UniqueVkHandle<VkRenderPass, RenderPassDeleter> _renderPass;

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
      VkFramebuffer frameBuffer);
  ~ActiveRenderPass();

  ActiveRenderPass& nextSubpass();

  // TODO: create IDrawable interface instead?
  template <typename TDrawable>
  ActiveRenderPass& draw(const TDrawable& object) {
    const DrawContext& context = this->_drawContext;
    object.draw(context);

    return *this;
  }

  ActiveRenderPass& setGlobalDescriptorSet(VkDescriptorSet set);

  ActiveRenderPass&
  setGlobalDescriptorSets(gsl::span<const VkDescriptorSet> sets);

  const DrawContext& getDrawContext() const { return this->_drawContext; }

  bool isLastSubpass() const {
    return this->_currentSubpass == this->_renderPass._subpasses.size() - 1;
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