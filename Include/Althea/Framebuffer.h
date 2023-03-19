#pragma once

#include "FrameContext.h"
#include "Library.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;

/**
 * @brief Creates a FrameBuffer from the specified attachment ImageViews.
 *
 * Attachments must be compatible with the provided render pass.
 */
class ALTHEA_API FrameBuffer {
public:
  FrameBuffer() = default;
  FrameBuffer(
      const Application& app,
      VkRenderPass renderPass,
      const VkExtent2D& extent,
      const std::vector<VkImageView>& attachmentViews);

  operator VkFramebuffer() const { return this->_frameBuffer; }

private:
  struct FrameBufferDeleter {
    void operator()(VkDevice device, VkFramebuffer frameBuffer) const;
  };

  UniqueVkHandle<VkFramebuffer, FrameBufferDeleter> _frameBuffer;
};

/**
 * @brief Creates a collection of FrameBuffers with the first attachment as a
 * swapchain image view and the subsequent attachments as the specified
 * attachment ImageViews.
 *
 * Attachments must be compatible with the provided render pass.
 */
class ALTHEA_API SwapChainFrameBufferCollection {
public:
  SwapChainFrameBufferCollection() = default;
  SwapChainFrameBufferCollection(
      const Application& app,
      VkRenderPass renderPass,
      const std::vector<VkImageView>& subsequentAttachmentViews);

  VkFramebuffer getCurrentFrameBuffer(const FrameContext& frame) const;

private:
  std::vector<FrameBuffer> _frameBuffers;
};
} // namespace AltheaEngine