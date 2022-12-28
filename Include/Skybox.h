#pragma once

#include "Cubemap.h"
#include "FrameContext.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <array>
#include <string>

namespace AltheaEngine {
class Application;
class GraphicsPipeline;
class GraphicsPipelineBuilder;

class Skybox {
public:
  static void buildPipeline(Application& app, GraphicsPipelineBuilder& builder);

  Skybox(
      Application& app, 
      const std::array<std::string, 6>& skyboxImagePaths);
  ~Skybox();

  void assignDescriptorSets(Application& app, GraphicsPipeline& pipeline);
  void updateUniforms(
      const glm::mat4& view, 
      const glm::mat4& projection, 
      const FrameContext& frame) const;
  void draw(
      const VkCommandBuffer& commandBuffer, 
      const VkPipelineLayout& pipelineLayout, 
      const FrameContext& frame) const;
private:
  VkDevice _device;

  Cubemap _cubemap;

  std::vector<VkDescriptorSet> _descriptorSets;
  std::vector<VkBuffer> _uniformBuffers;
  std::vector<VkDeviceMemory> _uniformBuffersMemory;
};
} // namespace AltheaEngine