#pragma once

#include "ConfigParser.h"
#include "Model.h"

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <cstdint>

class ShaderManager {
public:
  const VkShaderModule& getShaderModule(
      const VkDevice& device, 
      const std::string& shaderName);
  void destroy(const VkDevice& device);

private:
  struct ShaderModuleEntry {
    std::vector<char> code;
    VkShaderModule module;
  };

  std::unordered_map<std::string, ShaderModuleEntry> _shaders;
};

struct RenderPassCreateInfo {
  std::optional<std::string> vertexShader;
  std::optional<std::string> tessellationControlShader;
  std::optional<std::string> tessellationEvaluationShader;
  std::optional<std::string> geometryShader;
  std::optional<std::string> fragmentShader;
};

class Scene {
private:
  friend class RenderPass;

  // Buffer binding caches
  std::vector<VkBuffer> _vertexBuffers;
  std::vector<VkDeviceSize> _vertexBufferOffsets;
  std::vector<VkBuffer> _indexBuffers;

public:
  void addVertexBuffer(const VkBuffer& vertexBuffer);
  void addVertexBufferOffset(VkDeviceSize offset);
  void addIndexBuffer(const VkBuffer& indexBuffer);
  void addIndexBufferOffset(VkDeviceSize offset);
};

class RenderPass
{
public: 
  RenderPass(
      const std::string& name,
      const VkDevice& device,
      const VkPhysicalDevice& physicalDevice,
      const VkExtent2D& extent,
      const VkFormat& imageFormat,
      const ConfigParser& configParser,
      ShaderManager& shaderManager,
      const RenderPassCreateInfo& createInfo);

  void destroy(const VkDevice& device);

  bool wasSuccessful() const {
    return this->_success;
  }

  const VkRenderPass& getVulkanRenderPass() const {
    return this->_renderPass;
  }

  void runRenderPass(
      const VkCommandBuffer& commandBuffer,
      const VkFramebuffer& frameBuffer, 
      const VkExtent2D& extent) const;

private: 
  // TODO: should the pipeline and render pass be broken into separate classes??
  VkRenderPass _renderPass;
  VkPipeline _pipeline;
  VkPipelineLayout _pipelineLayout;
  bool _success;

  ModelManager _modelManager;
};

class RenderPassManager 
{
public:
  RenderPassManager(
      const VkDevice& device, 
      const VkPhysicalDevice& physicalDevice,
      const VkExtent2D& extent,
      const VkFormat& imageFormat,
      const ConfigParser& configParser);

  const RenderPass& find(const std::string& renderPassName) const;
  void destroy(const VkDevice& device);
private:
  ShaderManager _shaderManager;
  std::unordered_map<std::string, RenderPass> _renderPasses;
};
