#include "RenderPass.h"

#include "Utilities.h"

#include <stdexcept>

namespace {
  struct ShaderToCompile {
    std::string shaderPath = "";
    std::string compiledPath = "";
  };

  class ShadersListConfig : public ConfigCategory {
  public:
    std::vector<ShaderToCompile> shaderToCompile;

    ShadersListConfig()
      : ConfigCategory("SHADERS_LIST") {}

    void parseLine(const std::string& line) override {
      size_t splitIndex = line.find(" ");
      if (splitIndex != std::string::npos) {
        shaderToCompile.push_back({
          line.substr(0, splitIndex),
          line.substr(splitIndex + 1, line.length())});
      }
      else {
        shaderToCompile.push_back({
          "",
          line });
      }
    }
  };
};

ShaderManager::ShaderManager(const ConfigParser& configParser) {
  ShadersListConfig shadersList;

  configParser.parseCategory(shadersList);
  
}

void ShaderManager::_compileShader(const std::string& shaderPath, const std::string& compiledShaderPath) {
  
}

void ShaderManager::_loadShader(const std::string& compiledShaderPath) {
  this->_shaderBytecodes.push_back(Utilities::readFile(compiledShaderPath));
}

const std::vector<char>& ShaderManager::getShaderCode(
    uint32_t shaderId) const {
  if (shaderId < this->_shaderBytecodes.size()) {
    return this->_shaderBytecodes[shaderId];
  }
}

static VkShaderModule createShaderModule(const VkDevice& device, const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }

  return shaderModule;
}

RenderPass::RenderPass(const VkDevice& device, const VkExtent2D& extent, const RenderPassCreateInfo& createInfo)
    : _success(false) {
  if (!createInfo.vertexShaderPath) {
    return;
  }

  // TODO: currently hardcoded to _no_ vertex input, abstract vertex input
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.reserve(4);

  VkPipelineShaderStageCreateInfo& vertShaderStageInfo = shaderStages.emplace_back();
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = 
    createShaderModule(
      device,
      Utilities::readFile(createInfo.vertexShaderPath));
  vertShaderStageInfo.pName = "main";

  if (createInfo.geometryShaderPath) {
    VkPipelineShaderStageCreateInfo& geomShaderStageInfo = shaderStages.emplace_back();
    geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    geomShaderStageInfo.module =
      createShaderModule(
        device,
        Utilities::readFile(createInfo.geometryShaderPath));
    geomShaderStageInfo.pName = "main";
  }

  if (createInfo.fragmentShaderPath) {
    VkPipelineShaderStageCreateInfo& fragShaderStageInfo = shaderStages.emplace_back();
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module =
      createShaderModule(
        device,
        Utilities::readFile(createInfo.fragmentShaderPath));
    fragShaderStageInfo.pName = "main";
  }

  for (VkPipelineShaderStageCreateInfo& createInfo : shaderStages) {
    vkDestroyShaderModule(device, createInfo.module, nullptr);
  }
}

void RenderPass::destroy(const VkDevice& device) {

}