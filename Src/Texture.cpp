#include "Texture.h"

#include "Application.h"
#include "GlobalHeap.h"
#include "SingleTimeCommandBuffer.h"
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

  uint32_t mipCount = Utilities::computeMipCount(
                          static_cast<uint32_t>(image.width),
                          static_cast<uint32_t>(image.height));
  SamplerOptions samplerInfo(sampler, mipCount);
  mipCount = samplerInfo.mipCount;

  m_resource.sampler = Sampler(app, samplerInfo);

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
  if (mipCount > 1) {
    options.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  m_resource.image = Image(app, commandBuffer, mip0View, options);

  // Assume texture images are going to be used in a fragment shader
  m_resource.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  ImageViewOptions viewOptions{};
  viewOptions.format = options.format;
  viewOptions.mipCount = mipCount;

  m_resource.view = ImageView(app, m_resource.image, viewOptions);
}

void Texture::registerToHeap(GlobalHeap& heap) {
  m_resource.registerToTextureHeap(heap);
}
} // namespace AltheaEngine
