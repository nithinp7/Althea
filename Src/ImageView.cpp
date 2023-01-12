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
    VkImageAspectFlags aspectFlags)
    : _device(app.getDevice()) {

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

  if (vkCreateImageView(this->_device, &createInfo, nullptr, &this->_view) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create image view!");
  }
}

ImageView::ImageView(ImageView&& rhs) : _device(rhs._device), _view(rhs._view) {
  rhs._device = VK_NULL_HANDLE;
  rhs._view = VK_NULL_HANDLE;
}

ImageView& ImageView::operator=(ImageView&& rhs) {
  this->_device = rhs._device;
  this->_view = rhs._view;

  rhs._device = VK_NULL_HANDLE;
  rhs._view = VK_NULL_HANDLE;

  return *this;
}

ImageView::~ImageView() {
  if (this->_view != VK_NULL_HANDLE) {
    vkDestroyImageView(this->_device, this->_view, nullptr);
  }
}
} // namespace AltheaEngine