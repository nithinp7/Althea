#include "Cubemap.h"

#include "Application.h"
#include "Utilities.h"

#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/GltfReader.h>
#include <gsl/span>

#include <cstdint>
#include <stdexcept>

namespace AltheaEngine {
Cubemap::Cubemap(
    Application& app,
    const std::array<std::string, 6>& cubemapPaths,
    bool srgb) {
  std::array<CesiumGltf::ImageCesium, 6> cubemapImages;
  for (uint32_t i = 0; i < 6; ++i) {
    std::vector<char> rawImage = Utilities::readFile(cubemapPaths[i]);
    CesiumGltfReader::ImageReaderResult result =
        CesiumGltfReader::GltfReader::readImage(
            gsl::span<const std::byte>(
                reinterpret_cast<const std::byte*>(rawImage.data()),
                rawImage.size()),
            CesiumGltf::Ktx2TranscodeTargets{});
    if (!result.image) {
      throw std::runtime_error("Could not read cubemap image!");
    }

    cubemapImages[i] = std::move(*result.image);

    if (CesiumGltfReader::GltfReader::generateMipMaps(cubemapImages[i])) {
      throw std::runtime_error("Unable to generate mipmap for cubemap image!");
    }
  }

  this->_initCubemap(app, cubemapImages, srgb);
}

Cubemap::Cubemap(
    Application& app,
    const std::array<CesiumGltf::ImageCesium, 6>& images,
    bool srgb) {
  this->_initCubemap(app, images, srgb);
}

void Cubemap::_initCubemap(
    Application& app,
    const std::array<CesiumGltf::ImageCesium, 6>& cubemapImages,
    bool srgb) {
  // TODO: determine order expected by vulkan
  // Front, Back, Up, Down, Left Right?

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;

  uint32_t mipCount =
      static_cast<uint32_t>(cubemapImages[0].mipPositions.size());
  if (mipCount == 0) {
    mipCount = 1;
  }

  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(mipCount - 1);

  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy =
      app.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  this->_allocation = app.createCubemapImage(
      cubemapImages,
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);

  this->_sampler = Sampler(app, samplerInfo);
  this->_imageView = ImageView(
      app,
      this->_allocation.getImage(),
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
      mipCount,
      6,
      VK_IMAGE_VIEW_TYPE_CUBE,
      VK_IMAGE_ASPECT_COLOR_BIT);
}

} // namespace AltheaEngine