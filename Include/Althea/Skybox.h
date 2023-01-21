#pragma once

#include "Library.h"

#include "Cubemap.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "Material.h"
#include "SingleTimeCommandBuffer.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <string>

namespace AltheaEngine {
class Application;
class GraphicsPipelineBuilder;

class ALTHEA_API Skybox {
public:
  static void buildPipeline(Application& app, GraphicsPipelineBuilder& builder);

  Skybox(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      const std::array<std::string, 6>& skyboxImagePaths,
      bool srgb);

  void draw(const DrawContext& context) const;

  const std::shared_ptr<Cubemap>& getCubemap() const { return this->_pCubemap; }

private:
  VkDevice _device;
  std::shared_ptr<Cubemap> _pCubemap;
};
} // namespace AltheaEngine