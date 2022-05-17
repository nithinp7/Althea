#include "RenderPass.h"

#include "Utilities.h"

#include <stdexcept>
#include <unordered_map>

VkShaderModule ShaderManager::createShaderModule(
    const VkDevice& device, 
    const std::string& shaderName) {
  const std::vector<char>& code = this->_getShaderCode(shaderName);

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

const std::vector<char>& ShaderManager::_getShaderCode(
    const std::string& shaderName) {
  auto shaderIt = this->_shaders.find(shaderName);
  if (shaderIt != this->_shaders.end()) {
    return shaderIt->second;
  }

  auto newShader = 
      this->_shaders.emplace(
        shaderName,
        Utilities::readFile("../Shaders/" + shaderName + ".spv"));
  return newShader.first->second;
}

namespace {
class RenderPassConfig : public ConfigCategory {
public:
  std::unordered_map<std::string, RenderPassCreateInfo> createInfos;

  RenderPassConfig()
    : ConfigCategory("SHADER_PIPELINES") {}

  void parseLine(const std::string& line) override {
    size_t nextSpace = line.find(" ");
    if (nextSpace == std::string::npos) {
      return;
    }

    std::string tail(line);
    std::string token = tail.substr(0, nextSpace);
    tail = tail.substr(nextSpace + 1, tail.length());

    // Tokens are delimited by whitespace
    // The first token is the name of the pipeline.
    auto existingInfoIt = createInfos.find(token);
    if (existingInfoIt != createInfos.end()) {
      // A pipeline by the same name was already defined
      return;
    }

    RenderPassCreateInfo createInfo;

    // The rest of the tokens are names of shaders in the pipeline.
    nextSpace = tail.find(" ");
    while (nextSpace != std::string::npos) {
      token = tail.substr(0, nextSpace);
      tail = tail.substr(nextSpace + 1, tail.length());

      size_t extensionStart = token.rfind(".");
      if (extensionStart == std::string::npos) {
        continue;
      }

      std::string extension = token.substr(extensionStart, token.length());
      if (extension == ".vert") {
        createInfo.vertexShader = token;
      }
      else if (extension == ".tesc") {
        createInfo.tessellationControlShader = token;
      }
      else if (extension == ".tese") {
        createInfo.tessellationEvaluationShader = token;
      }
      else if (extension == ".geom") {
        createInfo.geometryShader = token;
      }
      else if (extension == ".frag") {
        createInfo.fragmentShader = token;
      }
    }

    createInfos.emplace(token, std::move(createInfo));
  }
};
} // namespace

RenderPass::RenderPass(
    const VkDevice& device,
    const VkExtent2D& extent,
    ShaderManager& shaderManager, 
    const RenderPassCreateInfo& createInfo)
    : _success(false) {
  if (!createInfo.vertexShader) {
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
      shaderManager.createShaderModule(
        device,
        *createInfo.vertexShader);
  vertShaderStageInfo.pName = "main";

  if (createInfo.geometryShader) {
    VkPipelineShaderStageCreateInfo& geomShaderStageInfo = shaderStages.emplace_back();
    geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    geomShaderStageInfo.module =
      shaderManager.createShaderModule(
        device,
        *createInfo.geometryShader);
    geomShaderStageInfo.pName = "main";
  }

  if (createInfo.fragmentShader) {
    VkPipelineShaderStageCreateInfo& fragShaderStageInfo = shaderStages.emplace_back();
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module =
      shaderManager.createShaderModule(
        device,
        *createInfo.fragmentShader);
    fragShaderStageInfo.pName = "main";
  }

  for (VkPipelineShaderStageCreateInfo& createInfo : shaderStages) {
    vkDestroyShaderModule(device, createInfo.module, nullptr);
  }
}

void RenderPass::destroy(const VkDevice& device) {

}