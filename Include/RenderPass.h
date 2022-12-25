#pragma once

#include "ConfigParser.h"
#include "Model.h"

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <cstdint>

namespace AltheaEngine {
class Application;
class ShaderManager;

struct RenderPassCreateInfo {
  std::optional<std::string> vertexShader;
  std::optional<std::string> tessellationControlShader;
  std::optional<std::string> tessellationEvaluationShader;
  std::optional<std::string> geometryShader;
  std::optional<std::string> fragmentShader;
};

class RenderPass
{
public: 
  RenderPass(
      const Application& app, 
      const std::string& name, 
      ShaderManager& shaderManager,
      const RenderPassCreateInfo& createInfo);
  ~RenderPass();

  const VkRenderPass& getVulkanRenderPass() const {
    return this->_renderPass;
  }

  void updateUniforms(
      const glm::mat4& view, const glm::mat4& projection, uint32_t currentFrame) const;

  void runRenderPass(
      const VkCommandBuffer& commandBuffer,
      const VkFramebuffer& frameBuffer, 
      const VkExtent2D& extent, 
      uint32_t currentFrame) const;

private: 
  VkDevice _device;

  // TODO: should the pipeline and render pass be broken into separate classes??
  VkRenderPass _renderPass;
  VkPipeline _pipeline;
  VkPipelineLayout _pipelineLayout;

  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;

  ModelManager _modelManager;
};

class RenderPassManager 
{
public:
  RenderPassManager(const Application& app);
  const RenderPass& find(const std::string& renderPassName) const;
private:
  ShaderManager _shaderManager;
  std::unordered_map<std::string, RenderPass> _renderPasses;
};
} // namespace AltheaEngine

