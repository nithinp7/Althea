#pragma once

#include "Application.h"
#include "Image.h"
#include "ImageView.h"
#include "Library.h"
#include "Sampler.h"
#include "SingleTimeCommandBuffer.h"

#include <vulkan/vulkan.h>

namespace CesiumGltf {
struct Model;
struct Texture;
struct ImageCesium;
struct Sampler;
} // namespace CesiumGltf

namespace AltheaEngine {
class ALTHEA_API Texture {
public:
  Texture(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture,
      bool srgb);
  Texture(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);

  VkImage getImage() const { return this->_image; }

  VkImageView getImageView() const { return this->_imageView; }

  VkSampler getSampler() const { return this->_sampler; }

private:
  void _initTexture(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const CesiumGltf::ImageCesium& image,
      const CesiumGltf::Sampler& sampler,
      bool srgb);

  Image _image;
  ImageView _imageView;
  Sampler _sampler;
};
} // namespace AltheaEngine
