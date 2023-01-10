#include "Texture.h"

#include <CesiumGltf/Image.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Texture.h>
#include <gsl/span>

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
  this->_device = app.getDevice();

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

  if (vkCreateSampler(
          this->_device,
          &samplerInfo,
          nullptr,
          &this->_textureSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create texture sampler!");
    return;
  }

  gsl::span<const std::byte> mips[11];
  uint32_t mipCount = 1;

  if (image.mipPositions.empty()) {
    mips[0] = gsl::span<const std::byte>(image.pixelData);
  } else {
    mipCount = static_cast<uint32_t>(image.mipPositions.size());
    if (mipCount > 11) {
      // Generous max mip count.
      mipCount = 11;
    }

    for (uint32_t i = 0; i < mipCount; ++i) {
      const CesiumGltf::ImageCesiumMipPosition& mipPos = image.mipPositions[i];
      mips[i] = gsl::span<const std::byte>(
          &image.pixelData[mipPos.byteOffset],
          mipPos.byteSize);
    }
  }

  app.createTextureImage(
      gsl::span<gsl::span<const std::byte>>(mips, mipCount),
      image.width,
      image.height,
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
      this->_textureImage,
      this->_textureImageMemory);

  this->_textureImageView = app.createImageView(
      this->_textureImage,
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
      mipCount,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_ASPECT_COLOR_BIT);
}

Texture::~Texture() {
  vkDestroySampler(this->_device, this->_textureSampler, nullptr);
  vkDestroyImageView(this->_device, this->_textureImageView, nullptr);

  vkDestroyImage(this->_device, this->_textureImage, nullptr);
  vkFreeMemory(this->_device, this->_textureImageMemory, nullptr);
}
} // namespace AltheaEngine
