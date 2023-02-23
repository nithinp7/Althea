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
    SingleTimeCommandBuffer& commandBuffer,
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
  }

  this->_initCubemap(app, commandBuffer, cubemapImages, srgb);
}

Cubemap::Cubemap(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::array<CesiumGltf::ImageCesium, 6>& images,
    bool srgb) {
  this->_initCubemap(app, commandBuffer, images, srgb);
}

void Cubemap::_initCubemap(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::array<CesiumGltf::ImageCesium, 6>& cubemapImages,
    bool srgb) {
  // TODO: determine order expected by vulkan
  // Front, Back, Up, Down, Left Right?
  std::vector<std::byte> buffer;

  size_t layerSize = cubemapImages[0].pixelData.size();
  if (!cubemapImages[0].mipPositions.empty()) {
    layerSize = cubemapImages[0].mipPositions[0].byteSize;
  }

  size_t totalSize = 6 * layerSize;
  buffer.resize(totalSize);

  // Pack all the first mips from each layer back-to-back.
  size_t offset = 0;
  for (uint32_t layerIndex = 0; layerIndex < 6; ++layerIndex) {
    memcpy(
        &buffer[offset],
        cubemapImages[layerIndex].pixelData.data(),
        layerSize);
    offset += layerSize;
  }

  ImageOptions options{};
  options.width = static_cast<uint32_t>(cubemapImages[0].width);
  options.height = static_cast<uint32_t>(cubemapImages[0].height);
  options.mipCount = Utilities::computeMipCount(options.width, options.height);
  options.format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
  options.layerCount = 6;
  options.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  options.createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

  this->_image = Image(app, commandBuffer, buffer, options);

  // Assume cubemap will be used in a fragment shader
  this->_image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
      
  SamplerOptions samplerInfo{};
  samplerInfo.mipCount = options.mipCount;

  this->_sampler = Sampler(app, samplerInfo);
  this->_imageView = ImageView(
      app,
      this->_image.getImage(),
      options.format,
      options.mipCount,
      6,
      VK_IMAGE_VIEW_TYPE_CUBE,
      VK_IMAGE_ASPECT_COLOR_BIT);
}

} // namespace AltheaEngine