#pragma once

#include "ConfigParser.h"

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <cstdint>

class ShaderManager {
public:
  ShaderManager(const ConfigParser& configParser);
  const std::vector<char>& getShaderCode(uint32_t shaderId) const;

private:
  void _compileShader(const std::string& shaderPath, const std::string& compiledShaderPath);
  void _loadShader(const std::string& compiledShaderPath);

  std::vector<std::vector<char>> _shaderBytecodes;
};

struct RenderPassCreateInfo {
  const VkDevice* pDevice;
  const VkExtent2D* pExtent;

  const char* vertexShaderPath = nullptr;
  // TODO: what are control and eval stages of tessellation
  //const char* tesselationShaderPath = nullptr;
  const char* geometryShaderPath = nullptr;
  const char* fragmentShaderPath = nullptr;
};

class RenderPass
{
public: 
  RenderPass(
      const VkDevice& device,
      const VkExtent2D& extent,
      const RenderPassCreateInfo& createInfo);

  void destroy(const VkDevice& device);

  bool wasSuccessful() const {
    return this->_success;
  }
private: 
  bool _success;
};

