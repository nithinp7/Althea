#include "ShaderManager.h"
#include "Utilities.h"

#include <stdexcept>

namespace AltheaEngine {
ShaderManager::ShaderManager(const VkDevice& device) 
    : _device(device) {}

ShaderManager::~ShaderManager() {
  for (const auto& shaderPair : this->_shaders) {
    vkDestroyShaderModule(this->_device, shaderPair.second.module, nullptr);
  }
}

const VkShaderModule& ShaderManager::getShaderModule(const std::string& shaderName) {
  auto shaderIt = this->_shaders.find(shaderName);
  if (shaderIt != this->_shaders.end()) {
    return shaderIt->second.module;
  }

  auto newShaderResult =
      this->_shaders.emplace(
        shaderName, 
        ShaderModuleEntry {
          Utilities::readFile("../Shaders/" + shaderName + ".spv"),
          VkShaderModule{}});
  
  ShaderModuleEntry& newShader = newShaderResult.first->second;
  
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = newShader.code.size();
  createInfo.pCode = 
      reinterpret_cast<const uint32_t*>(newShader.code.data());

  if (vkCreateShaderModule(this->_device, &createInfo, nullptr, &newShader.module) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }

  return newShader.module;
}
} // namespace AltheaEngine