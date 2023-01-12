#pragma once

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;

class ImageView {
public:
  ImageView(
      const Application& app, 
      VkImage image,
      VkFormat format,
      uint32_t mipCount,
      uint32_t layerCount,
      VkImageViewType type,
      VkImageAspectFlags aspectFlags);

  ImageView() = default;
  
  // Move-only semantics
  ImageView(ImageView&& rhs);
  ImageView& operator=(ImageView&& rhs);

  ImageView(const ImageView& rhs) = delete;
  ImageView& operator=(const ImageView& rhs) = delete;

  ~ImageView();

  VkImageView getImageView() const {
    return this->_view;
  }

private:
  VkDevice _device = VK_NULL_HANDLE;
  VkImageView _view = VK_NULL_HANDLE;
};
} // namespace AltheaEngine