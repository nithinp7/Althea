#pragma once

#include "ConfigParser.h"

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <cstdint>

class ShaderManager {
public:
  VkShaderModule createShaderModule(
      const VkDevice& device, 
      const std::string& shaderName);

private:
  std::unordered_map<std::string, std::vector<char>> _shaders;

  const std::vector<char>& _getShaderCode(const std::string& shaderName);
};

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
      const VkDevice& device,
      const VkExtent2D& extent,
      ShaderManager& shaderManager,
      const RenderPassCreateInfo& createInfo);

  void destroy(const VkDevice& device);

  bool wasSuccessful() const {
    return this->_success;
  }
private: 
  bool _success;
};

class RenderPassManager 
{
public:
  RenderPassManager(const ConfigParser& configParser);

private:
  std::unordered_map<std::string, RenderPass> _renderPasses;
};
