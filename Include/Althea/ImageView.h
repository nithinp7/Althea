#pragma once

#include "Library.h"

#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;

class ALTHEA_API ImageView {
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

  operator VkImageView() const { return this->_view; }

private:
  struct ImageViewDeleter {
    void operator()(VkDevice device, VkImageView imageView);
  };

  UniqueVkHandle<VkImageView, ImageViewDeleter> _view;
};
} // namespace AltheaEngine