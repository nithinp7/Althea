#include "RenderPass.h"

#include "Application.h"
#include "ImageView.h"

#include <stdexcept>

namespace AltheaEngine {
Subpass::Subpass(
    const Application& app,
    const VkExtent2D& extent,
    VkRenderPass renderPass,
    uint32_t subpassIndex,
    SubpassBuilder&& builder)
    : _pipeline(
          app,
          PipelineContext{renderPass, subpassIndex, extent},
          std::move(builder.pipelineBuilder)) {}

RenderPass::RenderPass(
    const Application& app,
    const VkExtent2D& extent,
    std::vector<Attachment>&& attachments,
    std::vector<SubpassBuilder>&& subpassBuilders)
    : _attachments(std::move(attachments)),
      _device(app.getDevice()),
      _extent(extent) {

  bool isCubemap = false;
  std::vector<VkAttachmentDescription> vkAttachments(this->_attachments.size());
  for (size_t i = 0; i < this->_attachments.size(); ++i) {
    const Attachment& attachment = this->_attachments[i];
    VkAttachmentDescription& vkAttachment = vkAttachments[i];

    bool forPresent = !attachment.frameBufferImageView;

    if (forPresent) {
      if (i > 0) {
        throw std::runtime_error(
            "Attachment for presentation must be the first attachment.");
      }

      this->_firstAttachmentFromSwapChain = true;
    }

    vkAttachment.format = attachment.format;
    vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // TODO: Configure if we actually want to clear it before loading. More
    // context in the next TODO comment below.
    vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    vkAttachment.storeOp = attachment.internalUsageOnly
                               ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                               : VK_ATTACHMENT_STORE_OP_STORE;

    vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // TODO: need a way to configure context for what the attachment
    // was doing before. E.g., we may be doing anti-aliasing with
    // previous frame buffers.
    vkAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // If this attachment will be presented, the final layout should reflect
    // that. Otherwise, the attachment may be used in subsequent render passes
    // or used as a render target.
    vkAttachment.finalLayout =
        forPresent ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        : (attachment.flags & ATTACHMENT_FLAG_DEPTH)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (attachment.flags & ATTACHMENT_FLAG_CUBEMAP) {
      isCubemap = true;
    }
  }

  std::vector<VkSubpassDescription> vkSubpasses;
  vkSubpasses.resize(subpassBuilders.size());
  std::vector<VkSubpassDependency> subpassDependencies;
  subpassDependencies.resize(subpassBuilders.size());

  std::vector<std::vector<VkAttachmentReference>> subpassAttachmentReferences;
  for (uint32_t subpassIndex = 0;
       subpassIndex < static_cast<uint32_t>(subpassBuilders.size());
       ++subpassIndex) {

    const SubpassBuilder& subpass = subpassBuilders[subpassIndex];
    VkSubpassDescription& vkSubpass = vkSubpasses[subpassIndex];

    std::vector<VkAttachmentReference>& vkColorAttachments =
        subpassAttachmentReferences.emplace_back();
    for (uint32_t attachmentIndex : subpass.colorAttachments) {
      vkColorAttachments.push_back(
          {attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    }

    std::optional<VkAttachmentReference> vkDepthAttachment = std::nullopt;
    if (subpass.depthAttachment) {
      vkDepthAttachment = {
          *subpass.depthAttachment,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    }

    // TODO: Support other types of pipelines
    vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkSubpass.colorAttachmentCount =
        static_cast<uint32_t>(vkColorAttachments.size());
    vkSubpass.pColorAttachments = vkColorAttachments.data();
    vkSubpass.pDepthStencilAttachment =
        vkDepthAttachment ? &*vkDepthAttachment : nullptr;

    VkPipelineStageFlags relevantStageFlags = 0;
    if (!subpass.colorAttachments.empty()) {
      relevantStageFlags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    if (subpass.depthAttachment) {
      relevantStageFlags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    VkSubpassDependency& dependency = subpassDependencies[subpassIndex];
    dependency.srcSubpass =
        (subpassIndex == 0) ? VK_SUBPASS_EXTERNAL : (subpassIndex - 1);
    dependency.dstSubpass = subpassIndex;
    dependency.srcStageMask = relevantStageFlags;
    dependency.srcAccessMask =
        (subpassIndex == 0)
            ? 0
            : subpassDependencies[subpassIndex - 1].dstAccessMask;
    dependency.dstStageMask = relevantStageFlags;

    dependency.dstAccessMask = 0;
    if (!subpass.colorAttachments.empty()) {
      dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (subpass.depthAttachment) {
      dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
  }

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
  renderPassInfo.pAttachments = vkAttachments.data();
  renderPassInfo.subpassCount = static_cast<uint32_t>(vkSubpasses.size());
  renderPassInfo.pSubpasses = vkSubpasses.data();
  renderPassInfo.dependencyCount =
      static_cast<uint32_t>(subpassDependencies.size());
  renderPassInfo.pDependencies = subpassDependencies.data();

  // Use the multi-view extension to render cubemaps
  std::vector<uint32_t> viewMasks;
  VkRenderPassMultiviewCreateInfo multiViewInfo{};
  multiViewInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
  multiViewInfo.subpassCount = renderPassInfo.subpassCount;
  multiViewInfo.pViewMasks = nullptr;
  multiViewInfo.correlationMaskCount = 0;
  multiViewInfo.pCorrelationMasks = nullptr;
  multiViewInfo.dependencyCount = 0;
  multiViewInfo.pViewOffsets = nullptr;

  if (isCubemap) {
    viewMasks.resize(renderPassInfo.subpassCount);
    for (uint32_t& mask : viewMasks) {
      mask = 0b111111;
    }

    multiViewInfo.pViewMasks = viewMasks.data();
    renderPassInfo.pNext = &multiViewInfo;
  }

  if (vkCreateRenderPass(
          this->_device,
          &renderPassInfo,
          nullptr,
          &this->_renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }

  // Create pipelines for all subpasses
  this->_subpasses.reserve(subpassBuilders.size());
  for (uint32_t subpassIndex = 0; subpassIndex < subpassBuilders.size();
       ++subpassIndex) {
    this->_subpasses.emplace_back(
        app,
        extent,
        this->_renderPass,
        subpassIndex,
        std::move(subpassBuilders[subpassIndex]));
  }

  // Create frame buffers for this render pass.
  if (this->_firstAttachmentFromSwapChain) {
    // This frame buffer will be used for presenting and the first attachment
    // will be from the swapchain. So we will need a different frame buffer for
    // each image in the swapchain.
    for (const ImageView& swapChainImageView : app.getSwapChainImageViews()) {
      this->_createFrameBuffer(swapChainImageView);
    }
  } else {
    this->_createFrameBuffer(std::nullopt);
  }
}

RenderPass::~RenderPass() {
  for (VkFramebuffer& framebuffer : this->_frameBuffers) {
    vkDestroyFramebuffer(this->_device, framebuffer, nullptr);
  }

  vkDestroyRenderPass(this->_device, this->_renderPass, nullptr);
}

void RenderPass::_createFrameBuffer(
    const std::optional<VkImageView>& swapChainImageView) {
  std::vector<VkImageView> attachmentImageViews(this->_attachments.size());
  for (size_t i = 0; i < this->_attachments.size(); ++i) {
    VkImageView& attachmentImageView = attachmentImageViews[i];
    const Attachment& attachment = this->_attachments[i];

    if (i == 0 && swapChainImageView) {
      attachmentImageView = *swapChainImageView;
    } else {
      attachmentImageView = *attachment.frameBufferImageView;
    }
  }

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = this->_renderPass;
  framebufferInfo.attachmentCount =
      static_cast<uint32_t>(attachmentImageViews.size());
  framebufferInfo.pAttachments = attachmentImageViews.data();
  framebufferInfo.width = this->_extent.width;
  framebufferInfo.height = this->_extent.height;
  framebufferInfo.layers = 1;

  VkFramebuffer& frameBuffer = this->_frameBuffers.emplace_back();
  if (vkCreateFramebuffer(
          this->_device,
          &framebufferInfo,
          nullptr,
          &frameBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create frame buffer!");
  }
}

ActiveRenderPass RenderPass::begin(
    const Application& app,
    const VkCommandBuffer& commandBuffer,
    const FrameContext& frame) {
  return ActiveRenderPass(*this, commandBuffer, frame, this->_extent);
}

ActiveRenderPass::ActiveRenderPass(
    const RenderPass& renderPass,
    const VkCommandBuffer& commandBuffer,
    const FrameContext& frame,
    const VkExtent2D& extent)
    : _currentSubpass(0),
      _renderPass(renderPass),
      _commandBuffer(commandBuffer),
      _frame(frame) {

  this->_drawContext._commandBuffer = commandBuffer;
  this->_drawContext._pFrame = &frame;

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = this->_renderPass._renderPass;

  if (this->_renderPass._firstAttachmentFromSwapChain) {
    renderPassInfo.framebuffer =
        this->_renderPass._frameBuffers[this->_frame.swapChainImageIndex];
  } else {
    renderPassInfo.framebuffer = this->_renderPass._frameBuffers[0];
  }

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;

  this->_clearValues.resize(this->_renderPass._attachments.size());
  for (size_t i = 0; i < this->_clearValues.size(); ++i) {
    this->_clearValues[i] = this->_renderPass._attachments[i].clearValue;
  }

  renderPassInfo.clearValueCount =
      static_cast<uint32_t>(this->_clearValues.size());
  renderPassInfo.pClearValues = this->_clearValues.data();

  const Subpass& currentSubpass =
      this->_renderPass._subpasses[this->_currentSubpass];

  this->_drawContext._pCurrentSubpass = &currentSubpass;

  // TODO: Support compute as well?
  vkCmdBeginRenderPass(
      this->_commandBuffer,
      &renderPassInfo,
      VK_SUBPASS_CONTENTS_INLINE);
  currentSubpass.getPipeline().bindPipeline(this->_commandBuffer);
}

ActiveRenderPass::~ActiveRenderPass() {
  vkCmdEndRenderPass(this->_commandBuffer);
}

ActiveRenderPass& ActiveRenderPass::nextSubpass() {
  ++this->_currentSubpass;
  if (this->_currentSubpass >= this->_renderPass._subpasses.size()) {
    throw std::runtime_error("Called nextSubpass with no remaining subpasses");
  }

  vkCmdNextSubpass(this->_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

  const Subpass& currentSubpass =
      this->_renderPass._subpasses[this->_currentSubpass];

  this->_drawContext._pCurrentSubpass = &currentSubpass;

  currentSubpass.getPipeline().bindPipeline(this->_commandBuffer);

  return *this;
}

ActiveRenderPass& ActiveRenderPass::setGlobalDescriptorSets(
    gsl::span<const VkDescriptorSet> sets) {
  if (sets.size() > 3) {
    throw std::runtime_error(
        "Attempting to set more than 3 global descriptor sets.");
  }

  this->_drawContext._globalDescriptorSetCount =
      static_cast<uint32_t>(sets.size());
  for (size_t i = 0; i < sets.size(); ++i) {
    this->_drawContext._descriptorSets[i] = sets[i];
  }

  return *this;
}

} // namespace AltheaEngine