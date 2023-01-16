#include "ImageView.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
ImageView::ImageView(
    const Application& app,
    VkImage image,
    VkFormat format,
    uint32_t mipCount,
    uint32_t layerCount,
    VkImageViewType type,
    VkImageAspectFlags aspectFlags) {

  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = type;
  createInfo.format = format;
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.subresourceRange.aspectMask = aspectFlags;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = mipCount;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = layerCount;

  VkDevice device = app.getDevice();
  VkImageView view;
  if (vkCreateImageView(device, &createInfo, nullptr, &view) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create image view!");
  }

  this->_view.set(device, view);
}

void ImageView::ImageViewDeleter::operator()(
    VkDevice device,
    VkImageView view) {
  vkDestroyImageView(device, view, nullptr);
}
} // namespace AltheaEngine