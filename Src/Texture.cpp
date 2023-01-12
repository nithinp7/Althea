#include "Texture.h"

#include <CesiumGltf/Image.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Texture.h>
#include <gsl/span>

#include <cstdint>
#include <memory>

namespace AltheaEngine {
Texture::Texture(
    const Application& app,
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool srgb) {
  if (texture.sampler < 0 || texture.sampler >= model.samplers.size() ||
      texture.source < 0 || texture.source >= model.images.size()) {
    return;
  }

  const CesiumGltf::Sampler& sampler = model.samplers[texture.sampler];
  const CesiumGltf::ImageCesium& image = model.images[texture.source].cesium;

  this->_initTexture(app, image, sampler, srgb);
}

Texture::Texture(
    const Application& app,
    const CesiumGltf::ImageCesium& image,
    const CesiumGltf::Sampler& sampler,
    bool srgb) {
  this->_initTexture(app, image, sampler, srgb);
}

void Texture::_initTexture(
    const Application& app,
    const CesiumGltf::ImageCesium& image,
    const CesiumGltf::Sampler& sampler,
    bool srgb) {
  // TODO: support compressed pixel formats
  if (image.compressedPixelFormat !=
      CesiumGltf::GpuCompressedPixelFormat::NONE) {
    return;
  }

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

  switch (sampler.wrapS) {
  case CesiumGltf::Sampler::WrapS::MIRRORED_REPEAT:
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    break;
  case CesiumGltf::Sampler::WrapS::REPEAT:
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    break;
  // case CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE:
  default:
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  };

  switch (sampler.wrapT) {
  case CesiumGltf::Sampler::WrapT::MIRRORED_REPEAT:
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    break;
  case CesiumGltf::Sampler::WrapT::REPEAT:
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    break;
  // case CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE:
  default:
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  };

  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  if (sampler.magFilter) {
    switch (*sampler.magFilter) {
    case CesiumGltf::Sampler::MagFilter::NEAREST:
      samplerInfo.magFilter = VK_FILTER_NEAREST;
      break;
    default:
      samplerInfo.magFilter = VK_FILTER_LINEAR;
    }
  } else {
    samplerInfo.magFilter = VK_FILTER_LINEAR;
  }

  bool useMipMaps = false;

  // Determine minification filter
  int32_t minFilter = sampler.minFilter.value_or(
      CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR);

  switch (minFilter) {
  case CesiumGltf::Sampler::MinFilter::NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    break;
  // case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
  // case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  // case CesiumGltf::Sampler::MinFilter::LINEAR:
  default:
    samplerInfo.minFilter = VK_FILTER_LINEAR;
  }

  // Determine mipmap mode.
  switch (minFilter) {
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
    useMipMaps = true;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    break;
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
    useMipMaps = true;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    break;
  case CesiumGltf::Sampler::MinFilter::NEAREST:
  case CesiumGltf::Sampler::MinFilter::LINEAR:
  default:
    useMipMaps = false;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }

  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (useMipMaps && !image.mipPositions.empty()) {
    samplerInfo.maxLod = static_cast<float>(image.mipPositions.size() - 1);
  }

  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy =
      app.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  this->_sampler = Sampler(app, samplerInfo);

  uint32_t mipCount = static_cast<uint32_t>(image.mipPositions.size());
  if (mipCount == 0) {
    mipCount = 1;
  }

  this->_allocation = app.createTextureImage(
      image,
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);

  this->_imageView = ImageView(
      app,
      this->_allocation.getImage(),
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
      mipCount,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_ASPECT_COLOR_BIT);
}
} // namespace AltheaEngine
