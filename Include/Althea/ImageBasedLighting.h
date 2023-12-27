#pragma once

#include "BindlessHandle.h"
#include "DescriptorSet.h"
#include "ImageResource.h"
#include "Library.h"
#include "ResourcesAssignment.h"
#include "SingleTimeCommandBuffer.h"

#include <cstdint>
#include <string>

namespace AltheaEngine {
class Application;
class GlobalHeap;

struct ALTHEA_API IBLHandles {
  uint32_t environmentMapHandle;
  uint32_t prefilteredMapHandle;
  uint32_t irradianceMapHandle;
  uint32_t brdfLutHandle;
};

struct ALTHEA_API IBLResources {
  ImageResource environmentMap{};
  ImageHandle environmentMapHandle{};
  ImageResource prefilteredMap{};
  ImageHandle prefilteredMapHandle{};
  ImageResource irradianceMap{};
  ImageHandle irradianceMapHandle{};
  ImageResource brdfLut{};
  ImageHandle brdfLutHandle{};

  void bind(ResourcesAssignment& assignment);
  void registerToHeap(GlobalHeap& heap);
  IBLHandles getHandles() const {
    return {
        environmentMapHandle.index,
        prefilteredMapHandle.index,
        irradianceMapHandle.index,
        brdfLutHandle.index};
  }
};

namespace ImageBasedLighting {
IBLResources createResources(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    const std::string& envMapName);

void buildLayout(
    DescriptorSetLayoutBuilder& layoutBuilder,
    VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL);
} // namespace ImageBasedLighting
} // namespace AltheaEngine