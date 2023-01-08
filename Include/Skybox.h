#pragma once

#include "Cubemap.h"
#include "DrawContext.h"
#include "FrameContext.h"
#include "Material.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <string>

namespace AltheaEngine {
class Application;
class GraphicsPipelineBuilder;
class DescriptorSetAllocator;

class Skybox {
public:
  static void buildPipeline(Application& app, GraphicsPipelineBuilder& builder);

  Skybox(
      Application& app,
      const std::array<std::string, 6>& skyboxImagePaths,
      bool srgb,
      DescriptorSetAllocator& materialAllocator);
  ~Skybox();

  void updateUniforms(
      const glm::mat4& view,
      const glm::mat4& projection,
      const FrameContext& frame) const;
  void draw(const DrawContext& context) const;

  const std::shared_ptr<Cubemap>& getCubemap() const { return this->_pCubemap; }

private:
  VkDevice _device;

  std::shared_ptr<Cubemap> _pCubemap;

  std::unique_ptr<Material> _pMaterial;

  std::vector<VkBuffer> _uniformBuffers;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;
};
} // namespace AltheaEngine