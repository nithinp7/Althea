#include "Framebuffer.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {

FrameBuffer::FrameBuffer(
    const Application& app,
    VkRenderPass renderPass,
    const VkExtent2D& extent,
    const std::vector<VkImageView>& attachmentViews) {

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount =
      static_cast<uint32_t>(attachmentViews.size());
  framebufferInfo.pAttachments = attachmentViews.data();
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = 1;

  VkDevice device = app.getDevice();
  VkFramebuffer frameBuffer;
  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create frame buffer!");
  }

  this->_frameBuffer.set(device, frameBuffer);
}

void FrameBuffer::FrameBufferDeleter::operator()(
    VkDevice device,
    VkFramebuffer frameBuffer) const {
  vkDestroyFramebuffer(device, frameBuffer, nullptr);
}

SwapChainFrameBufferCollection::SwapChainFrameBufferCollection(
    const Application& app,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& subsequentAttachmentViews) {
  const std::vector<ImageView>& swapChainViews = app.getSwapChainImageViews();
  const VkExtent2D& extent = app.getSwapChainExtent();

  this->_frameBuffers.reserve(swapChainViews.size());

  std::vector<VkImageView> scratchViews;
  scratchViews.resize(1 + subsequentAttachmentViews.size());
  for (const ImageView& swapChainImageView : app.getSwapChainImageViews()) {
    scratchViews[0] = swapChainImageView;
    for (size_t i = 0; i < subsequentAttachmentViews.size(); ++i) {
      scratchViews[i + 1] = subsequentAttachmentViews[i];
    }

    this->_frameBuffers.emplace_back(app, renderPass, extent, scratchViews);
  }
}

VkFramebuffer SwapChainFrameBufferCollection::getCurrentFrameBuffer(
    const FrameContext& frame) const {
  return this->_frameBuffers[frame.swapChainImageIndex];
}
} // namespace AltheaEngine