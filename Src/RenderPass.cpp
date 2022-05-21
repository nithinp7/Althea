#include "RenderPass.h"

#include "Utilities.h"

#include <stdexcept>
#include <unordered_map>

const VkShaderModule& ShaderManager::getShaderModule(
    const VkDevice& device, 
    const std::string& shaderName) {
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

  if (vkCreateShaderModule(device, &createInfo, nullptr, &newShader.module) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }

  return newShader.module;
}

void ShaderManager::destroy(const VkDevice& device) {
  for (const auto& shaderPair : this->_shaders) {
    vkDestroyShaderModule(device, shaderPair.second.module, nullptr);
  }
}

RenderPass::RenderPass(
    const VkDevice& device,
    const VkExtent2D& extent,
    const VkFormat& imageFormat,
    ShaderManager& shaderManager, 
    const RenderPassCreateInfo& createInfo)
    : _success(false) {
  if (!createInfo.vertexShader) {
    return;
  }

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) extent.width;
  viewport.height = (float) extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // FIXED FUNCTION STAGES
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

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

/*
  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();*/

  // PROGRAMMABLE STAGES
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.reserve(4);

  VkPipelineShaderStageCreateInfo& vertShaderStageInfo = shaderStages.emplace_back();
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = 
      shaderManager.getShaderModule(
        device,
        *createInfo.vertexShader);
  vertShaderStageInfo.pName = "main";

  if (createInfo.geometryShader) {
    VkPipelineShaderStageCreateInfo& geomShaderStageInfo = shaderStages.emplace_back();
    geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    geomShaderStageInfo.module =
      shaderManager.getShaderModule(
        device,
        *createInfo.geometryShader);
    geomShaderStageInfo.pName = "main";
  }

  if (createInfo.fragmentShader) {
    VkPipelineShaderStageCreateInfo& fragShaderStageInfo = shaderStages.emplace_back();
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module =
      shaderManager.getShaderModule(
        device,
        *createInfo.fragmentShader);
    fragShaderStageInfo.pName = "main";
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &this->_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = imageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &this->_renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = this->_pipelineLayout;
  pipelineInfo.renderPass = this->_renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(
        device, 
        VK_NULL_HANDLE, 
        1, 
        &pipelineInfo, 
        nullptr, 
        &this->_pipeline)) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }
}

void RenderPass::runRenderPass(
    const VkCommandBuffer& commandBuffer,
    const VkFramebuffer& frameBuffer, 
    const VkExtent2D& extent) const {

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = this->_renderPass;
  renderPassInfo.framebuffer = frameBuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_pipeline);
  // TODO: generalize vertex count
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);
}

void RenderPass::destroy(const VkDevice& device) {
  vkDestroyPipeline(device, this->_pipeline, nullptr);
  vkDestroyPipelineLayout(device, this->_pipelineLayout, nullptr);
  vkDestroyRenderPass(device, this->_renderPass, nullptr);
}

namespace {
class RenderPassConfig : public ConfigCategory {
public:
  std::unordered_map<std::string, RenderPassCreateInfo> createInfos;

  RenderPassConfig()
    : ConfigCategory("SHADER_PIPELINES") {}

  void parseLine(const std::string& line) override {
    size_t split = line.find(" ");
    if (split == std::string::npos) {
      return;
    }

    std::string tail(line);
    std::string token = tail.substr(0, split);

    // Tokens are delimited by whitespace
    // The first token is the name of the pipeline.
    auto existingInfoIt = createInfos.find(token);
    if (existingInfoIt != createInfos.end()) {
      // A pipeline by the same name was already defined
      return;
    }

    auto createInfoResult =
      createInfos.emplace(token, RenderPassCreateInfo{});
    RenderPassCreateInfo& createInfo = 
        createInfoResult.first->second;

    // The rest of the tokens are names of shaders in the pipeline.
    do {
      tail = tail.substr(split + 1, tail.length());
      split = std::min(tail.find(" "), tail.size());
      token = tail.substr(0, split);

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
    } while (split < tail.size());
  }
};
} // namespace

RenderPassManager::RenderPassManager(
    const VkDevice& device,
    const VkExtent2D& extent,
    const VkFormat& imageFormat,
    const ConfigParser& configParser) {
  RenderPassConfig renderPassConfig;
  configParser.parseCategory(renderPassConfig);

  for (const auto& infoPair : renderPassConfig.createInfos) {
    this->_renderPasses.emplace(
        infoPair.first, 
        RenderPass(
          device,
          extent,
          imageFormat,
          this->_shaderManager,
          infoPair.second));
  }
}

const RenderPass& RenderPassManager::find(const std::string& renderPassName) const {
  return this->_renderPasses.find(renderPassName)->second;
}

void RenderPassManager::destroy(const VkDevice& device) {
  for (auto& renderPassPair : this->_renderPasses) {
    renderPassPair.second.destroy(device);
  }

  this->_shaderManager.destroy(device);
}