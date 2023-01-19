#pragma once

#include "Library.h"

#include "Allocator.h"
#include "ImageView.h"
#include "Sampler.h"

#include <CesiumGltf/ImageCesium.h>
#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace AltheaEngine {
class Application;

class ALTHEA_API Cubemap {
public:
  Cubemap(
      Application& app,
      const std::array<std::string, 6>& cubemapPaths,
      bool srgb);
  Cubemap(
      Application& app,
      const std::array<CesiumGltf::ImageCesium, 6>& images,
      bool srgb);

  VkImage getImage() const { return this->_allocation.getImage(); }

  VkImageView getImageView() const { return this->_imageView; }

  VkSampler getSampler() const { return this->_sampler; }

private:
  void _initCubemap(
      Application& app,
      const std::array<CesiumGltf::ImageCesium, 6>& images,
      bool srgb);

  ImageAllocation _allocation;

  ImageView _imageView;
  Sampler _sampler;
};
} // namespace AltheaEngine