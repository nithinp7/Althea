#include "RenderPass2.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Subpass::Subpass(
    const Application& app, 
    const PipelineContext& context, 
    const SubpassBuilder& builder) :
  _pipeline(app, context, builder.graphicsPipeline) {}

RenderPass2::RenderPass2(
    const Application& app,
    std::vector<Attachment>&& attachments, 
    std::vector<SubpassBuilder>&& subpassBuilders) :
    _attachments(std::move(attachments)),
    _device(app.getDevice()) {

  std::vector<VkAttachmentDescription> vkAttachments;
  for (const Attachment& attachment : this->_attachments) {
    VkAttachmentDescription& vkAttachment = vkAttachments.emplace_back();

    vkAttachment.format = attachment.format;
    vkAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    vkAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vkAttachment.storeOp = 
        attachment.forPresentation ? 
          VK_ATTACHMENT_STORE_OP_STORE :
          VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkAttachment.finalLayout = 
        attachment.forPresentation ? 
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 
          attachment.type == AttachmentType::Depth ? 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : 
            VK_IMAGE_LAYOUT_UNDEFINED;
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

    // TODO: Support other types of pipelines
    vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkSubpass.colorAttachmentCount = 
        static_cast<uint32_t>(subpass.colorAttachments.size());
    vkSubpass.pColorAttachments = subpass.colorAttachments.data();
    vkSubpass.pDepthStencilAttachment = 
        subpass.depthAttachment ? 
          &*subpass.depthAttachment : nullptr;

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

  // Actually create pipelines for all subpasses 
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
}

RenderPass2::~RenderPass2() {
  vkDestroyRenderPass(this->_device, this->_renderPass, nullptr);
}
} // namespace AltheaEngine