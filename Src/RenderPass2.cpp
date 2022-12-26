#include "RenderPass2.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Subpass::Subpass(
    const Application& app, 
    const PipelineContext& context, 
    const SubpassBuilder& builder) :
  _pipeline(app, context, builder.pipelineBuilder) {}

RenderPass2::RenderPass2(
    const Application& app,
    std::vector<Attachment>&& attachments, 
    std::vector<SubpassBuilder>&& subpassBuilders) :
    _attachments(std::move(attachments)),
    _device(app.getDevice()) {

  // TODO: make this more readable

  bool firstAttachmentFromSwapChain = false;

  std::vector<VkAttachmentDescription> vkAttachments(this->_attachments.size());
  for (size_t i = 0; i < this->_attachments.size(); ++i) {
    const Attachment& attachment = this->_attachments[i];
    VkAttachmentDescription& vkAttachment = vkAttachments.emplace_back();

    bool forPresent = !attachment.frameBufferImageView;

    if (forPresent) {
      if (i > 0) {
        throw std::runtime_error("Attachment for presentation must be the first attachment.");
      }

      firstAttachmentFromSwapChain = true;
    }

    vkAttachment.format = attachment.format;
    vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // TODO: Configure if we actually want to clear it before loading. More
    // context in the next TODO comment below.
    vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    vkAttachment.storeOp = 
        attachment.internalUsageOnly ? 
          VK_ATTACHMENT_STORE_OP_DONT_CARE :
          VK_ATTACHMENT_STORE_OP_STORE;

    vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // TODO: need a way to configure context for what the attachment
    // was doing before. E.g., we may be doing anti-aliasing with
    // previous frame buffers.
    vkAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // If this attachment will be presented, the final layout should reflect that.
    // Otherwise, the attachment may be used in subsequent render passes or used
    // as a render target.
    vkAttachment.finalLayout = 
        forPresent ? 
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 
          attachment.type == AttachmentType::Depth ? 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }

  std::vector<VkSubpassDescription> vkSubpasses;
  vkSubpasses.resize(subpassBuilders.size());
  std::vector<VkSubpassDependency> subpassDependencies;
  subpassDependencies.resize(subpassBuilders.size());
  for (uint32_t subpassIndex = 0; 
       subpassIndex < static_cast<uint32_t>(subpassBuilders.size()); 
       ++subpassIndex) {

    const SubpassBuilder& subpass = subpassBuilders[subpassIndex];
    VkSubpassDescription& vkSubpass = vkSubpasses[subpassIndex];

    std::vector<VkAttachmentReference> vkColorAttachments;
    for (uint32_t attachmentIndex : subpass.colorAttachments) {
      vkColorAttachments.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    }

    std::optional<VkAttachmentReference> vkDepthAttachment = std::nullopt;
    if (subpass.depthAttachment) {
      vkDepthAttachment = {*subpass.depthAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    }

    // TODO: Support other types of pipelines
    vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkSubpass.colorAttachmentCount = 
        static_cast<uint32_t>(vkColorAttachments.size());
    vkSubpass.pColorAttachments = vkColorAttachments.data();
    vkSubpass.pDepthStencilAttachment = 
        vkDepthAttachment ? 
          &*vkDepthAttachment : nullptr;

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
        (subpassIndex == 0) ? 0 : subpassDependencies[subpassIndex-1].dstAccessMask;
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
  renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
  renderPassInfo.pDependencies = subpassDependencies.data();

  if (vkCreateRenderPass(this->_device, &renderPassInfo, nullptr, &this->_renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }

  // Create pipelines for all subpasses 
  this->_subpasses.reserve(subpassBuilders.size());
  for (uint32_t subpassIndex = 0; 
       subpassIndex < subpassBuilders.size();
       ++subpassIndex) {
    this->_subpasses.push_back(
        Subpass(
          app,  
          PipelineContext{this->_renderPass, subpassIndex},
          subpassBuilders[subpassIndex]));
  }

  // Create frame buffers for this render pass.

  // TODO: Support frame buffer extents other than swap chain image size?
  VkExtent2D extent = app.getSwapChainExtent();
  if (firstAttachmentFromSwapChain) {
    // This frame buffer will be used for presenting and the first attachment
    // will be from the swapchain. So we will need a different frame buffer for
    // each image in the swapchain.
    const std::vector<VkImageView>& swapChainImageViews = 
        app.getSwapChainImageViews();
    for (const VkImageView& swapChainImageView : swapChainImageViews) {
      this->_createFrameBuffer(extent, swapChainImageView);
    }
  } else {
    this->_createFrameBuffer(extent, std::nullopt);
  }
}

RenderPass2::~RenderPass2() {
  for (VkFramebuffer& framebuffer : this->_frameBuffers) {
    vkDestroyFramebuffer(this->_device, framebuffer, nullptr);
  }

  vkDestroyRenderPass(this->_device, this->_renderPass, nullptr);
}

void RenderPass2::_createFrameBuffer(
    const VkExtent2D& extent,
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
  framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentImageViews.size());
  framebufferInfo.pAttachments = attachmentImageViews.data();
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
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
} // namespace AltheaEngine