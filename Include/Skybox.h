#pragma once

#include "Cubemap.h"
#include "FrameContext.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <string>


namespace AltheaEngine {
class Application;
class GraphicsPipelineBuilder;
class DescriptorSet;
class DescriptorSetAllocator;

class Skybox {
public:
  static void buildPipeline(Application& app, GraphicsPipelineBuilder& builder);

  Skybox(
      Application& app,
      const std::array<std::string, 6>& skyboxImagePaths,
      DescriptorSetAllocator& materialAllocator);
  ~Skybox();

  void updateUniforms(
      const glm::mat4& view,
      const glm::mat4& projection,
      const FrameContext& frame) const;
  void draw(
      const VkCommandBuffer& commandBuffer,
      const VkPipelineLayout& pipelineLayout,
      const FrameContext& frame) const;

  const std::shared_ptr<Cubemap>& getCubemap() const { return this->_pCubemap; }

private:
  void
  _createMaterial(Application& app, DescriptorSetAllocator& materialAllocator);

  VkDevice _device;

  std::shared_ptr<Cubemap> _pCubemap;

  std::vector<DescriptorSet> _descriptorSets;
  std::vector<VkBuffer> _uniformBuffers;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;
};
} // namespace AltheaEngine