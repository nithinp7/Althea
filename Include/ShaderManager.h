#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace AltheaEngine {
class ShaderManager {
public:
  ShaderManager(const VkDevice& device);
  ~ShaderManager();

  const VkShaderModule& getShaderModule(const std::string& shaderName);

private:
  struct ShaderModuleEntry {
    std::vector<char> code;
    VkShaderModule module;
  };

  VkDevice _device;
  std::unordered_map<std::string, ShaderModuleEntry> _shaders;
};
} // namespace AltheaEngine