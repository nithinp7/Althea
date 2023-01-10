#include "Cubemap.h"

#include "Application.h"
#include "Utilities.h"

#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/GltfReader.h>
#include <gsl/span>

#include <stdexcept>

namespace AltheaEngine {
Cubemap::Cubemap(
    Application& app,
    const std::array<std::string, 6>& cubemapPaths,
    bool srgb)
    : _device(app.getDevice()) {
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
  }

  this->_initCubemap(app, cubemapImages, srgb);
}

Cubemap::Cubemap(
    Application& app,
    const std::array<CesiumGltf::ImageCesium, 6>& images,
    bool srgb)
    : _device(app.getDevice()) {
  this->_initCubemap(app, images, srgb);
}

Cubemap::~Cubemap() {
  vkDestroySampler(this->_device, this->_imageSampler, nullptr);
  vkDestroyImageView(this->_device, this->_imageView, nullptr);

  vkDestroyImage(this->_device, this->_image, nullptr);
  vkFreeMemory(this->_device, this->_imageMemory, nullptr);
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

  // TODO: revisit mipmapping
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy =
      app.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  if (vkCreateSampler(
          this->_device,
          &samplerInfo,
          nullptr,
          &this->_imageSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create cubemap sampler!");
    return;
  }

  std::array<gsl::span<const std::byte>, 6> cubemapBuffers;
  for (uint32_t i = 0; i < 6; ++i) {
    cubemapBuffers[i] = gsl::span<const std::byte>(cubemapImages[i].pixelData);
  }

  app.createCubemapImage(
      cubemapBuffers,
      cubemapImages[0].width,
      cubemapImages[0].height,
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
      this->_image,
      this->_imageMemory);

  this->_imageView = app.createImageView(
      this->_image,
      srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM,
      1,
      6,
      VK_IMAGE_VIEW_TYPE_CUBE,
      VK_IMAGE_ASPECT_COLOR_BIT);
}

} // namespace AltheaEngine