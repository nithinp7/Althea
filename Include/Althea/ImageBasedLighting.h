#pragma once

#include "DescriptorSet.h"
#include "ImageResource.h"
#include "Library.h"
#include "ResourcesAssignment.h"
#include "SingleTimeCommandBuffer.h"

#include <string>

namespace AltheaEngine {
class Application;

struct ALTHEA_API IBLResources {
  ImageResource environmentMap{};
  ImageResource prefilteredMap{};
  ImageResource irradianceMap{};
  ImageResource brdfLut{};

  void bind(ResourcesAssignment& assignment);
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