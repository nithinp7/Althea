#include "ImageBasedLighting.h"

#include "Application.h"
#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"
#include "Utilities.h"

#include <CesiumGltf/ImageCesium.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {

void IBLResources::bind(ResourcesAssignment& assignment) {
  assignment
      .bindTexture(this->environmentMap.view, this->environmentMap.sampler)
      .bindTexture(this->prefilteredMap.view, this->prefilteredMap.sampler)
      .bindTexture(this->irradianceMap.view, this->irradianceMap.sampler)
      .bindTexture(this->brdfLut.view, this->brdfLut.sampler);
}

namespace ImageBasedLighting {
IBLResources createResources(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::string& envMapName) {
  IBLResources resources{};

  // Environment map
  CesiumGltf::ImageCesium envMapImg = Utilities::loadHdri(
      GEngineDirectory + "/Content/HDRI_Skybox/" + envMapName + ".hdr");

  ImageOptions envMapOptions{};
  envMapOptions.width = static_cast<uint32_t>(envMapImg.width);
  envMapOptions.height = static_cast<uint32_t>(envMapImg.height);
  envMapOptions.mipCount = 1;
  // Utilities::computeMipCount(envMapOptions.width, envMapOptions.height);
  envMapOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  envMapOptions.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT;
  resources.environmentMap.image =
      Image(app, commandBuffer, envMapImg.pixelData, envMapOptions);

  resources.environmentMap.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  SamplerOptions envSamplerOptions{};
  envSamplerOptions.mipCount = envMapOptions.mipCount;
  envSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  envSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  envSamplerOptions.minFilter = VK_FILTER_LINEAR;
  envSamplerOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  resources.environmentMap.sampler = Sampler(app, envSamplerOptions);

  // TODO: create straight from image details?
  resources.environmentMap.view = ImageView(
      app,
      resources.environmentMap.image.getImage(),
      envMapOptions.format,
      envMapOptions.mipCount,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_ASPECT_COLOR_BIT);

  // Prefiltered map
  std::vector<CesiumGltf::ImageCesium> prefilteredMips;
  // TODO: parameterize prefiltered mip count??
  prefilteredMips.reserve(5);
  for (uint32_t i = 1; i < 6; ++i) {
    prefilteredMips.push_back(Utilities::loadHdri(
        GEngineDirectory + "/Content/PrecomputedMaps/" + envMapName +
        "/Prefiltered" + std::to_string(i) + ".hdr"));
  }

  ImageOptions prefilteredMapOptions{};
  prefilteredMapOptions.width = static_cast<uint32_t>(prefilteredMips[0].width);
  prefilteredMapOptions.height =
      static_cast<uint32_t>(prefilteredMips[0].height);
  prefilteredMapOptions.mipCount = 5;
  prefilteredMapOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  prefilteredMapOptions.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT;

  resources.prefilteredMap.image = Image(app, prefilteredMapOptions);
  resources.prefilteredMap.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

  for (uint32_t i = 0; i < 5; ++i) {
    resources.prefilteredMap.image
        .uploadMip(app, commandBuffer, prefilteredMips[i].pixelData, i);
  }

  resources.prefilteredMap.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  SamplerOptions prefMapSamplerOptions{};
  prefMapSamplerOptions.mipCount = prefilteredMapOptions.mipCount;
  prefMapSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  prefMapSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  prefMapSamplerOptions.minFilter = VK_FILTER_LINEAR;
  prefMapSamplerOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  resources.prefilteredMap.sampler = Sampler(app, prefMapSamplerOptions);
  resources.prefilteredMap.view = ImageView(
      app,
      resources.prefilteredMap.image.getImage(),
      prefilteredMapOptions.format,
      prefilteredMapOptions.mipCount,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_ASPECT_COLOR_BIT);

  // Irradiance map
  CesiumGltf::ImageCesium irrMapImg = Utilities::loadHdri(
      GEngineDirectory + "/Content/PrecomputedMaps/" + envMapName +
      "/IrradianceMap.hdr");

  ImageOptions irrMapOptions{};
  irrMapOptions.width = static_cast<uint32_t>(irrMapImg.width);
  irrMapOptions.height = static_cast<uint32_t>(irrMapImg.height);
  // Generate mips?
  irrMapOptions.mipCount = 1;
  irrMapOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  irrMapOptions.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT;

  resources.irradianceMap.image =
      Image(app, commandBuffer, irrMapImg.pixelData, irrMapOptions);

  resources.irradianceMap.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  SamplerOptions irrSamplerOptions{};
  irrSamplerOptions.mipCount = irrMapOptions.mipCount;
  irrSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  irrSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  irrSamplerOptions.minFilter = VK_FILTER_LINEAR;

  resources.irradianceMap.sampler = Sampler(app, irrSamplerOptions);
  resources.irradianceMap.view = ImageView(
      app,
      resources.irradianceMap.image.getImage(),
      irrMapOptions.format,
      irrMapOptions.mipCount,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_ASPECT_COLOR_BIT);

  // BRDF LUT
  CesiumGltf::ImageCesium brdf = Utilities::loadPng(
      GEngineDirectory + "/Content/PrecomputedMaps/brdf_lut.png");

  ImageOptions brdfOptions{};
  brdfOptions.width = static_cast<uint32_t>(brdf.width);
  brdfOptions.height = static_cast<uint32_t>(brdf.height);
  brdfOptions.format = VK_FORMAT_R8G8B8A8_UNORM;
  brdfOptions.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  brdfOptions.mipCount = 1;

  resources.brdfLut.image =
      Image(app, commandBuffer, brdf.pixelData, brdfOptions);

  resources.brdfLut.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  SamplerOptions brdfSamplerOptions{};
  brdfSamplerOptions.mipCount = brdfOptions.mipCount;
  brdfSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  brdfSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  brdfSamplerOptions.minFilter = VK_FILTER_LINEAR;

  resources.brdfLut.sampler = Sampler(app, brdfSamplerOptions);
  resources.brdfLut.view = ImageView(
      app,
      resources.brdfLut.image.getImage(),
      brdfOptions.format,
      1,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      VK_IMAGE_ASPECT_COLOR_BIT);

  return resources;
}

void buildLayout(DescriptorSetLayoutBuilder& layoutBuilder) {
  layoutBuilder
      // Add slot for environmentMap
      .addTextureBinding()
      // Add slot for prefiltered map
      .addTextureBinding()
      // Add slot for irradianceMap
      .addTextureBinding()
      // BRDF LUT
      .addTextureBinding();
}
} // namespace ImageBasedLighting
} // namespace AltheaEngine