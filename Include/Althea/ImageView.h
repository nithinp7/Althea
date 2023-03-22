#pragma once

#include "Library.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

struct ALTHEA_API ImageViewOptions {
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  uint32_t mipBias = 0;
  uint32_t mipCount = 1;
  uint32_t baseLayer = 0;
  uint32_t layerCount = 1;
  VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
  VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
};

class ALTHEA_API ImageView {
public:
  ImageView(
      const Application& app,
      VkImage image,
      const ImageViewOptions& options);
  ImageView() = default;

  const ImageViewOptions& getOptions() const { return this->_options; }

  operator VkImageView() const { return this->_view; }

private:
  ImageViewOptions _options;

  struct ImageViewDeleter {
    void operator()(VkDevice device, VkImageView imageView);
  };

  UniqueVkHandle<VkImageView, ImageViewDeleter> _view;
};
} // namespace AltheaEngine