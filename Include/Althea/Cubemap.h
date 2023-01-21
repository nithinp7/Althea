#pragma once

#include "Library.h"

#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"
#include "SingleTimeCommandBuffer.h"

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
      SingleTimeCommandBuffer& commandBuffer,
      const std::array<std::string, 6>& cubemapPaths,
      bool srgb);
  Cubemap(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::array<CesiumGltf::ImageCesium, 6>& images,
      bool srgb);

  VkImage getImage() const { return this->_image.getImage(); }

  VkImageView getImageView() const { return this->_imageView; }

  VkSampler getSampler() const { return this->_sampler; }

private:
  void _initCubemap(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::array<CesiumGltf::ImageCesium, 6>& images,
      bool srgb);

  Image _image;
  ImageView _imageView;
  Sampler _sampler;
};
} // namespace AltheaEngine