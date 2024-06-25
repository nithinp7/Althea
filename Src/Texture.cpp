#include "Texture.h"

#include "Utilities.h"

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
    SingleTimeCommandBuffer& commandBuffer,
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool srgb) {
  if (texture.source < 0 || texture.source >= model.images.size()) {
    return;
  }

  CesiumGltf::Sampler sampler{};
  if (texture.sampler >= 0 && texture.sampler < model.samplers.size())
    sampler = model.samplers[texture.sampler];

  const CesiumGltf::ImageCesium& image = model.images[texture.source].cesium;

  this->_initTexture(app, commandBuffer, image, sampler, srgb);
}

Texture::Texture(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const CesiumGltf::ImageCesium& image,
    const CesiumGltf::Sampler& sampler,
    bool srgb) {
  this->_initTexture(app, commandBuffer, image, sampler, srgb);
}

void Texture::_initTexture(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const CesiumGltf::ImageCesium& image,
    const CesiumGltf::Sampler& sampler,
    bool srgb) {
  // TODO: support compressed pixel formats
  if (image.compressedPixelFormat !=
      CesiumGltf::GpuCompressedPixelFormat::NONE) {
    return;
  }

  SamplerOptions samplerInfo{};

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

  uint32_t mipCount = useMipMaps ? Utilities::computeMipCount(
                                       static_cast<uint32_t>(image.width),
                                       static_cast<uint32_t>(image.height))
                                 : 1;

  samplerInfo.mipCount = mipCount;

  this->_sampler = Sampler(app, samplerInfo);

  gsl::span<const std::byte> mip0View(image.pixelData);
  if (!image.mipPositions.empty()) {
    const CesiumGltf::ImageCesiumMipPosition& mip0pos = image.mipPositions[0];
    mip0View = gsl::span<const std::byte>(
        &image.pixelData[mip0pos.byteOffset],
        mip0pos.byteSize);
  }

  ImageOptions options{};
  options.width = static_cast<uint32_t>(image.width);
  options.height = static_cast<uint32_t>(image.height);
  options.mipCount = mipCount;
  options.format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
  options.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (useMipMaps) {
    options.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  this->_image = Image(app, commandBuffer, mip0View, options);

  // Assume texture images are going to be used in a fragment shader
  this->_image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  ImageViewOptions viewOptions{};
  viewOptions.format = options.format;
  viewOptions.mipCount = mipCount;

  this->_imageView = ImageView(app, this->_image, viewOptions);
}
} // namespace AltheaEngine
