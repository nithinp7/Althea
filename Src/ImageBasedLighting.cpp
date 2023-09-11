#include "ImageBasedLighting.h"

#include "Application.h"
#include "ComputePipeline.h"
#include "Image.h"
#include "ImageView.h"
#include "Sampler.h"
#include "Utilities.h"

#include <CesiumGltf/ImageCesium.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <filesystem>
#include <stdexcept>

namespace AltheaEngine {

void IBLResources::bind(ResourcesAssignment& assignment) {
  assignment
      .bindTexture(this->environmentMap.view, this->environmentMap.sampler)
      .bindTexture(this->prefilteredMap.view, this->prefilteredMap.sampler)
      .bindTexture(this->irradianceMap.view, this->irradianceMap.sampler)
      .bindTexture(this->brdfLut.view, this->brdfLut.sampler);
}

namespace ImageBasedLighting {
// TODO: allow loading envmaps from alternate locations

namespace {
void saveHdriImage(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    Image& image,
    const std::string& path) {
  uint32_t width = image.getOptions().width;
  uint32_t height = image.getOptions().height;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  BufferAllocation stagingBuffer = BufferUtilities::createBuffer(
      app,
      width * height * 32 * 4,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      stagingAllocInfo);

  image.copyMipToBuffer(commandBuffer, stagingBuffer.getBuffer(), 0, 0);

  // Write out image and delete the staging buffer once the one-time GPU work is
  // complete
  commandBuffer.addPostCompletionTask(
      [width,
       height,
       path,
       pStagingBuffer = new BufferAllocation(std::move(stagingBuffer))]() {
        size_t bufferSize = pStagingBuffer->getInfo().size;
        std::vector<std::byte> buffer;
        buffer.resize(bufferSize);

        void* pStaging = pStagingBuffer->mapMemory();
        std::memcpy(buffer.data(), pStaging, bufferSize);
        pStagingBuffer->unmapMemory();

        Utilities::saveHdri(
            path,
            static_cast<int>(width),
            static_cast<int>(height),
            buffer);

        delete pStagingBuffer;
      });
}

struct GenIrradiancePass {
  std::unique_ptr<DescriptorSetAllocator> materialAllocator{};
  std::unique_ptr<DescriptorSet> material{};

  ComputePipeline pipeline{};
};

struct PrefilterEnvMapPasses {
  std::unique_ptr<DescriptorSetAllocator> materialAllocator{};
  std::vector<std::unique_ptr<DescriptorSet>> materials;

  ComputePipeline pipeline{};
};

struct ImageDetailsPushConstants {
  float width;
  float height;
};

struct PrefilterEnvMapPushConstants {
  float envMapWidth;
  float envMapHeight;
  float width;
  float height;
  float roughness;
};

struct IBLPrecomputeResources {
  ImageResource environmentMap;
  std::vector<ImageResource> preFilteredMap;
  ImageResource irradianceMap;
};

void precomputeResources(
    const Application& app,
    const std::string& envMapName) {

  SingleTimeCommandBuffer commandBuffer(app);

  // Create target directory in case it does not exist
  std::string precomputedDir =
      GEngineDirectory + "/Content/PrecomputedMaps/" + envMapName;
  std::filesystem::create_directory(
      std::filesystem::path(precomputedDir.c_str()));

  // Initialize resources
  IBLPrecomputeResources* pResources = new IBLPrecomputeResources();

  // Environment map
  CesiumGltf::ImageCesium envMapImg = Utilities::loadHdri(
      GEngineDirectory + "/Content/HDRI_Skybox/" + envMapName + ".hdr");

  ImageOptions imageOptions{};
  imageOptions.width = static_cast<uint32_t>(envMapImg.width);
  imageOptions.height = static_cast<uint32_t>(envMapImg.height);
  imageOptions.mipCount =
      Utilities::computeMipCount(imageOptions.width, imageOptions.height);
  imageOptions.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  imageOptions.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                       VK_IMAGE_USAGE_SAMPLED_BIT;
  pResources->environmentMap.image =
      Image(app, commandBuffer, envMapImg.pixelData, imageOptions);

  SamplerOptions samplerOptions{};
  samplerOptions.mipCount = imageOptions.mipCount;
  samplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerOptions.minFilter = VK_FILTER_LINEAR;
  samplerOptions.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  pResources->environmentMap.sampler = Sampler(app, samplerOptions);

  ImageViewOptions viewOptions{};
  viewOptions.format = imageOptions.format;
  viewOptions.mipCount = imageOptions.mipCount;

  pResources->environmentMap.view =
      ImageView(app, pResources->environmentMap.image, viewOptions);

  // Create pre-filtered environment maps
  pResources->preFilteredMap.reserve(5);
  SamplerOptions mipSamplerOptions{};
  mipSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  mipSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  mipSamplerOptions.mipCount = 1;

  ImageViewOptions mipViewOptions{};
  mipViewOptions.format = viewOptions.format;
  for (uint32_t mipIndex = 1; mipIndex < 6; ++mipIndex) {
    ImageOptions mipOptions = imageOptions;
    mipOptions.mipCount = 1;
    mipOptions.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    mipOptions.width >>= mipIndex;
    mipOptions.height >>= mipIndex;

    ImageResource& mipImage = pResources->preFilteredMap.emplace_back();
    mipImage.image = Image(app, mipOptions);
    mipImage.sampler = Sampler(app, mipSamplerOptions);
    mipImage.view = ImageView(app, mipImage.image, mipViewOptions);
  }

  // Create device-only resource for irradiance map
  GenIrradiancePass* pGenIrrPass = new GenIrradiancePass();

  ImageOptions irrMapOptions{};
  irrMapOptions.width = imageOptions.width;
  irrMapOptions.height = imageOptions.height;
  irrMapOptions.format = imageOptions.format;
  irrMapOptions.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_STORAGE_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  pResources->irradianceMap.image = Image(app, irrMapOptions);
  pResources->irradianceMap.sampler = Sampler(app, samplerOptions);

  ImageViewOptions irrViewOptions = mipViewOptions;
  pResources->irradianceMap.view =
      ImageView(app, pResources->irradianceMap.image, irrViewOptions);

  // Init irradiance map generation pass

  DescriptorSetLayoutBuilder genIrrResourcesLayout{};
  genIrrResourcesLayout
      // Environment map input
      .addTextureBinding(VK_SHADER_STAGE_COMPUTE_BIT)
      // Irradiance map output
      .addStorageImageBinding();

  pGenIrrPass->materialAllocator =
      std::make_unique<DescriptorSetAllocator>(app, genIrrResourcesLayout, 1);
  pGenIrrPass->material = std::make_unique<DescriptorSet>(
      pGenIrrPass->materialAllocator->allocate());
  pGenIrrPass->material
      ->assign()
      // Bind environment map input
      .bindTextureDescriptor(
          pResources->environmentMap.view,
          pResources->environmentMap.sampler)
      // Bind irradiance map output
      .bindStorageImage(
          pResources->irradianceMap.view,
          pResources->irradianceMap.sampler);

  ComputePipelineBuilder computeIrrBuilder;
  computeIrrBuilder.setComputeShader(
      GEngineDirectory + "/Shaders/IBL_Precompute/GenIrradianceMap.comp");
  computeIrrBuilder.layoutBuilder
      .addDescriptorSet(pGenIrrPass->materialAllocator->getLayout())
      .addPushConstants<ImageDetailsPushConstants>(VK_SHADER_STAGE_COMPUTE_BIT);

  pGenIrrPass->pipeline = ComputePipeline(app, std::move(computeIrrBuilder));

  // Init environment map filtering pass
  PrefilterEnvMapPasses* pPrefilterPass = new PrefilterEnvMapPasses();

  DescriptorSetLayoutBuilder prefilterEnvResourcesLayout{};
  prefilterEnvResourcesLayout
      // Environment map input
      .addTextureBinding(VK_SHADER_STAGE_COMPUTE_BIT)
      // Filtered env mip output
      .addStorageImageBinding();

  pPrefilterPass->materialAllocator = std::make_unique<DescriptorSetAllocator>(
      app,
      prefilterEnvResourcesLayout,
      5);

  pPrefilterPass->materials.reserve(5);
  for (uint32_t i = 0; i < 5; ++i) {
    auto& pMaterial =
        pPrefilterPass->materials.emplace_back(std::make_unique<DescriptorSet>(
            pPrefilterPass->materialAllocator->allocate()));
    pMaterial
        ->assign()
        // Environment map
        .bindTextureDescriptor(
            pResources->environmentMap.view,
            pResources->environmentMap.sampler)
        // Current prefiltered mip index
        .bindStorageImage(
            pResources->preFilteredMap[i].view,
            pResources->preFilteredMap[i].sampler);
  }

  ComputePipelineBuilder computePrefilteredEnvBuilder{};
  computePrefilteredEnvBuilder.setComputeShader(
      GEngineDirectory + "/Shaders/IBL_Precompute/PrefilterEnvMap.comp");
  computePrefilteredEnvBuilder.layoutBuilder
      .addDescriptorSet(pPrefilterPass->materialAllocator->getLayout())
      .addPushConstants<PrefilterEnvMapPushConstants>(
          VK_SHADER_STAGE_COMPUTE_BIT);

  pPrefilterPass->pipeline =
      ComputePipeline(app, std::move(computePrefilteredEnvBuilder));

  // All resources and compute passes are now set up, now generate
  // the irradiance and prefiltered maps

  pResources->environmentMap.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_ACCESS_SHADER_READ_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  // Generate irradiance map
  pResources->irradianceMap.image.transitionLayout(
      commandBuffer,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_ACCESS_SHADER_WRITE_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  pGenIrrPass->pipeline.bindPipeline(commandBuffer);

  VkDescriptorSet material = pGenIrrPass->material->getVkDescriptorSet();
  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      pGenIrrPass->pipeline.getLayout(),
      0,
      1,
      &material,
      0,
      nullptr);

  const ImageOptions& imageDetails =
      pResources->environmentMap.image.getOptions();
  ImageDetailsPushConstants genIrrConstants{};
  genIrrConstants.width = static_cast<float>(imageDetails.width);
  genIrrConstants.height = static_cast<float>(imageDetails.height);
  vkCmdPushConstants(
      commandBuffer,
      pGenIrrPass->pipeline.getLayout(),
      VK_SHADER_STAGE_COMPUTE_BIT,
      0,
      sizeof(ImageDetailsPushConstants),
      &genIrrConstants);

  uint32_t groupCountX = imageDetails.width / 16;
  uint32_t groupCountY = imageDetails.height / 16;
  vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

  saveHdriImage(
      app,
      commandBuffer,
      pResources->irradianceMap.image,
      precomputedDir + "/IrradianceMap.hdr");

  // Generate prefiltered environment map mips.
  pPrefilterPass->pipeline.bindPipeline(commandBuffer);
  for (uint32_t i = 0; i < 5; ++i) {
    ImageResource& mip = pResources->preFilteredMap[i];
    uint32_t width = mip.image.getOptions().width;
    uint32_t height = mip.image.getOptions().height;

    mip.image.transitionLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    VkDescriptorSet material =
        pPrefilterPass->materials[i]->getVkDescriptorSet();
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pPrefilterPass->pipeline.getLayout(),
        0,
        1,
        &material,
        0,
        nullptr);

    PrefilterEnvMapPushConstants prefilterEnvConstants{};
    prefilterEnvConstants.envMapWidth =
        static_cast<float>(pResources->environmentMap.image.getOptions().width);
    prefilterEnvConstants.envMapHeight = static_cast<float>(
        pResources->environmentMap.image.getOptions().height);
    prefilterEnvConstants.width = static_cast<float>(width);
    prefilterEnvConstants.height = static_cast<float>(height);
    prefilterEnvConstants.roughness = static_cast<float>(i) / 4.0f;

    vkCmdPushConstants(
        commandBuffer,
        pPrefilterPass->pipeline.getLayout(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(PrefilterEnvMapPushConstants),
        &prefilterEnvConstants);

    groupCountX = width / 16;
    groupCountY = height / 16;
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

    saveHdriImage(
        app,
        commandBuffer,
        mip.image,
        precomputedDir + "/Prefiltered" + std::to_string(i + 1) + ".hdr");
  }

  // Add post-completion task to delete all intermediary resources and
  // pipelines.
  commandBuffer.addPostCompletionTask(
      [pResources, pGenIrrPass, pPrefilterPass]() {
        delete pResources;
        delete pGenIrrPass;
        delete pPrefilterPass;
      });
}
} // namespace

IBLResources createResources(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::string& envMapName) {
  IBLResources resources{};

  std::string envMapFileName =
      GEngineDirectory + "/Content/HDRI_Skybox/" + envMapName + ".hdr";
  if (!Utilities::checkFileExists(envMapFileName)) {
    throw std::runtime_error("Specified environment map does not exist!");
  }

  // One or more of the expected precomputed maps or irradiance map is missing
  // so we need to precompute them from the environment map.
  bool needToPrecomputeIBL = false;

  std::string prefilteredMapNames[5];
  for (int i = 0; i < 5; ++i) {
    prefilteredMapNames[i] = GEngineDirectory + "/Content/PrecomputedMaps/" +
                             envMapName + "/Prefiltered" +
                             std::to_string(i + 1) + ".hdr";
    needToPrecomputeIBL |= !Utilities::checkFileExists(prefilteredMapNames[i]);
  }

  std::string irradianceMapName = GEngineDirectory +
                                  "/Content/PrecomputedMaps/" + envMapName +
                                  "/IrradianceMap.hdr";
  needToPrecomputeIBL |= !Utilities::checkFileExists(irradianceMapName);

  if (needToPrecomputeIBL) {
    precomputeResources(app, envMapName);
  }

  // Environment map
  CesiumGltf::ImageCesium envMapImg = Utilities::loadHdri(envMapFileName);

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

  ImageViewOptions envViewOptions{};
  envViewOptions.format = envMapOptions.format;
  envViewOptions.mipCount = envMapOptions.mipCount;
  resources.environmentMap.view =
      ImageView(app, resources.environmentMap.image, envViewOptions);

  // Prefiltered map
  std::vector<CesiumGltf::ImageCesium> prefilteredMips;
  // TODO: parameterize prefiltered mip count??
  prefilteredMips.reserve(5);
  for (uint32_t i = 0; i < 5; ++i) {
    prefilteredMips.push_back(Utilities::loadHdri(prefilteredMapNames[i]));
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

  ImageViewOptions prefViewOptions{};
  prefViewOptions.format = prefilteredMapOptions.format;
  prefViewOptions.mipCount = prefilteredMapOptions.mipCount;
  resources.prefilteredMap.view =
      ImageView(app, resources.prefilteredMap.image, prefViewOptions);

  // Irradiance map
  CesiumGltf::ImageCesium irrMapImg = Utilities::loadHdri(irradianceMapName);

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
  irrSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  irrSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  irrSamplerOptions.minFilter = VK_FILTER_LINEAR;

  resources.irradianceMap.sampler = Sampler(app, irrSamplerOptions);

  ImageViewOptions irrViewOptions{};
  irrViewOptions.format = irrMapOptions.format;
  irrViewOptions.mipCount = irrMapOptions.mipCount;
  resources.irradianceMap.view =
      ImageView(app, resources.irradianceMap.image, irrViewOptions);

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
  brdfSamplerOptions.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  brdfSamplerOptions.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  brdfSamplerOptions.minFilter = VK_FILTER_LINEAR;

  resources.brdfLut.sampler = Sampler(app, brdfSamplerOptions);

  ImageViewOptions brdfViewOptions{};
  brdfViewOptions.format = brdfOptions.format;
  resources.brdfLut.view =
      ImageView(app, resources.brdfLut.image, brdfViewOptions);

  return resources;
}

void buildLayout(DescriptorSetLayoutBuilder& layoutBuilder, VkShaderStageFlags shaderStages) {
  layoutBuilder
      // Add slot for environmentMap
      .addTextureBinding(shaderStages)
      // Add slot for prefiltered map
      .addTextureBinding(shaderStages)
      // Add slot for irradianceMap
      .addTextureBinding(shaderStages)
      // BRDF LUT
      .addTextureBinding(shaderStages);
}
} // namespace ImageBasedLighting
} // namespace AltheaEngine