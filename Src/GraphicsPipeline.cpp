#include "GraphicsPipeline.h"
#include "Application.h"
#include "ShaderManager.h"

#include <stdexcept>

namespace AltheaEngine {


GraphicsPipeline::GraphicsPipeline(
    const Application& app, 
    const PipelineContext& context,
    const GraphicsPipelineBuilder& builder) : 
    _device(app.getDevice()) {

  // VIEWPORT, SCISSOR, ETC

  const VkExtent2D& extent = app.getSwapChainExtent();

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

  VkPipelineViewportStateCreateInfo viewportStateInfo{};
  viewportStateInfo.sType = 
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateInfo.viewportCount = 1;
  viewportStateInfo.pViewports = &viewport;
  viewportStateInfo.scissorCount = 1;
  viewportStateInfo.pScissors = &scissor;


  // BINDINGS, DESCRIPTOR SETS, DESCRIPTOR POOLS, PIPELINE LAYOUT, ETC

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount = 
      static_cast<uint32_t>(builder._descriptorSetLayoutBindings.size());
  descriptorSetLayoutInfo.pBindings = builder._descriptorSetLayoutBindings.data();

  if (vkCreateDescriptorSetLayout(
          this->_device, 
          &descriptorSetLayoutInfo, 
          nullptr, 
          &this->_descriptorSetLayout) 
        != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }

  uint32_t descriptorSetCount = context.primitiveCount * app.getMaxFramesInFlight();

  std::vector<VkDescriptorPoolSize> poolSizes;
  for (const VkDescriptorSetLayoutBinding& binding : 
       builder._descriptorSetLayoutBindings) {
    VkDescriptorPoolSize& poolSize = poolSizes.emplace_back();
    poolSize.type = binding.descriptorType;
    poolSize.descriptorCount = descriptorSetCount * binding.descriptorCount;
  }
  
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = descriptorSetCount;
  
  if (builder._hasBoundConstants) {
    VkDescriptorPoolInlineUniformBlockCreateInfo inlineDescriptorPoolCreateInfo{};
    inlineDescriptorPoolCreateInfo.sType = 
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO;
    inlineDescriptorPoolCreateInfo.maxInlineUniformBlockBindings = 
        descriptorSetCount;

    poolInfo.pNext = &inlineDescriptorPoolCreateInfo;
  }

  if (vkCreateDescriptorPool(
        this->_device, 
        &poolInfo, 
        nullptr, 
        &this->_descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool!");
  }

  // TODO: descriptor set allocator
  std::vector<VkDescriptorSetLayout> layouts(descriptorSetCount, this->_descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = this->_descriptorPool;
  allocInfo.descriptorSetCount = descriptorSetCount;
  allocInfo.pSetLayouts = layouts.data();

  this->_descriptorSets.resize(descriptorSetCount);
  if (vkAllocateDescriptorSets(this->_device, &allocInfo, this->_descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &this->_descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(
        this->_device, 
        &pipelineLayoutInfo, 
        nullptr, 
        &this->_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }


  // VERTEX INPUT, ATTRIBUTES, ETC
  
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &builder._bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 
      static_cast<uint32_t>(builder._attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = 
      builder._attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  
  switch (builder._primitiveType) {
    case PrimitiveType::TRIANGLES:
      inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      break;
    case PrimitiveType::LINES:
      inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      break;
    case PrimitiveType::POINTS:
      inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
      break;
  };

  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;


  // RASTERIZATION, MULTISAMPLE, COLOR BLENDING, ETC
  
  // TODO: support transform feedback before rasterizer step (rasterizerDiscardEnable)
  VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
  rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerInfo.depthClampEnable = VK_FALSE;
  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;

  switch (builder._primitiveType) {
    case PrimitiveType::TRIANGLES:
      rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
      break;
    case PrimitiveType::LINES:
      rasterizerInfo.polygonMode = VK_POLYGON_MODE_LINE;
      break;
    case PrimitiveType::POINTS:
      rasterizerInfo.polygonMode = VK_POLYGON_MODE_POINT;
      break;
  };
  
  rasterizerInfo.lineWidth = builder._lineWidth;
  rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizerInfo.depthBiasEnable = VK_FALSE;
  rasterizerInfo.depthBiasConstantFactor = 0.0f;
  rasterizerInfo.depthBiasClamp = 0.0f;
  rasterizerInfo.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
  multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingInfo.sampleShadingEnable = VK_FALSE;
  multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisamplingInfo.minSampleShading = 1.0f;
  multisamplingInfo.pSampleMask = nullptr;
  multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
  multisamplingInfo.alphaToOneEnable = VK_FALSE;

  // TODO: can this be generalized in a useful way?
  // Make alpha blending optional?
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
  colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingInfo.logicOpEnable = VK_FALSE;
  colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlendingInfo.attachmentCount = 1;
  colorBlendingInfo.pAttachments = &colorBlendAttachment;
  colorBlendingInfo.blendConstants[0] = 0.0f; // Optional
  colorBlendingInfo.blendConstants[1] = 0.0f; // Optional
  colorBlendingInfo.blendConstants[2] = 0.0f; // Optional
  colorBlendingInfo.blendConstants[3] = 0.0f; // Optional


  // DEPTH STENCIL
  
  // Only gets used if this->_depthTest is true
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
  depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilInfo.depthTestEnable = VK_TRUE;
  depthStencilInfo.depthWriteEnable = VK_TRUE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.minDepthBounds = 0.0f; // Not used
  depthStencilInfo.maxDepthBounds = 1.0f; // Not used
  depthStencilInfo.stencilTestEnable = VK_FALSE;
  depthStencilInfo.front = {};
  depthStencilInfo.back = {};


  // DYNAMIC STATES

  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount = 
      static_cast<uint32_t>(builder._dynamicStates.size());
  dynamicStateInfo.pDynamicStates = builder._dynamicStates.data();


  // CREATE THE GRAPHICS PIPELINE
  
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 
      static_cast<uint32_t>(builder._shaderStages.size());
  pipelineInfo.pStages = builder._shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  pipelineInfo.pViewportState = &viewportStateInfo;
  pipelineInfo.pRasterizationState = &rasterizerInfo;
  pipelineInfo.pMultisampleState = &multisamplingInfo;
  pipelineInfo.pDepthStencilState = builder._depthTest ? &depthStencilInfo : nullptr;
  pipelineInfo.pColorBlendState = &colorBlendingInfo;
  pipelineInfo.pDynamicState = &dynamicStateInfo;
  pipelineInfo.layout = this->_pipelineLayout;
  pipelineInfo.renderPass = context.renderPass;
  pipelineInfo.subpass = context.subpassIndex;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;
  
  if (vkCreateGraphicsPipelines(
        this->_device, 
        VK_NULL_HANDLE, 
        1, 
        &pipelineInfo, 
        nullptr, 
        &this->_pipeline)) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }
}

GraphicsPipeline::~GraphicsPipeline() {
  vkDestroyDescriptorPool(this->_device, this->_descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(this->_device, this->_descriptorSetLayout, nullptr);
  vkDestroyPipeline(this->_device, this->_pipeline, nullptr);
  vkDestroyPipelineLayout(this->_device, this->_pipelineLayout, nullptr);
}

uint32_t GraphicsPipelineBuilder::addTextureBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT) {
  uint32_t bindingIndex = 
      static_cast<uint32_t>(this->_descriptorSetLayoutBindings.size());
  VkDescriptorSetLayoutBinding& binding = 
      this->_descriptorSetLayoutBindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return bindingIndex;
}

uint32_t GraphicsPipelineBuilder::addUniformBufferBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL) {
  uint32_t bindingIndex = 
      static_cast<uint32_t>(this->_descriptorSetLayoutBindings.size());
  VkDescriptorSetLayoutBinding& binding = 
      this->_descriptorSetLayoutBindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return bindingIndex;
}

void GraphicsPipelineBuilder::addComputeShader(
    ShaderManager& shaderManager,
    const std::string& shaderPath) {
  throw std::runtime_error("Compute shaders not yet supported!");
}

void GraphicsPipelineBuilder::addVertexShader(
    ShaderManager& shaderManager, 
    const std::string& shaderPath) {
  VkPipelineShaderStageCreateInfo& vertShaderStageInfo = 
      this->_shaderStages.emplace_back();
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = 
      shaderManager.getShaderModule(shaderPath);
  vertShaderStageInfo.pName = "main";
}

void GraphicsPipelineBuilder::addTessellationControlShader(
    ShaderManager& shaderManager, 
    const std::string& shaderPath) {
  throw std::runtime_error("Tessellation shaders not yet supported!");
}

void GraphicsPipelineBuilder::addTessellationEvaluationShader(
    ShaderManager& shaderManager, 
    const std::string& shaderPath) {
  throw std::runtime_error("Tessellation shaders not yet supported!");
}

void GraphicsPipelineBuilder::addGeometryShader(
    ShaderManager& shaderManager, 
    const std::string& shaderPath) {
  throw std::runtime_error("Geometry shaders not yet supported!");
  // VkPipelineShaderStageCreateInfo& geomShaderStageInfo = 
  //    this->_shaderStages.emplace_back();
  // geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  // geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
  // geomShaderStageInfo.module =
  //   shaderManager.getShaderModule(shaderPath);
  // geomShaderStageInfo.pName = "main";
}

void GraphicsPipelineBuilder::addFragmentShader(
    ShaderManager& shaderManager, 
    const std::string& shaderPath) {
  VkPipelineShaderStageCreateInfo& fragShaderStageInfo = 
      this->_shaderStages.emplace_back();
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module =
    shaderManager.getShaderModule(shaderPath);
  fragShaderStageInfo.pName = "main";
}

uint32_t GraphicsPipelineBuilder::addVertexAttribute(
    VertexAttributeType attributeType, 
    uint32_t offset) {
  VkFormat format;
  switch(attributeType) {
    case VertexAttributeType::INT:
      format = VK_FORMAT_R32_SINT;
      break;
    case VertexAttributeType::FLOAT:
      format = VK_FORMAT_R32_SFLOAT;
      break;
    case VertexAttributeType::VEC2:
      format = VK_FORMAT_R32G32_SFLOAT;
      break;
    case VertexAttributeType::VEC3:
      format = VK_FORMAT_R32G32B32_SFLOAT;
      break;
    case VertexAttributeType::VEC4:
      format = VK_FORMAT_R32G32B32A32_SFLOAT;
      break;
  };
  
  uint32_t attributeId = static_cast<uint32_t>(
      this->_attributeDescriptions.size());
  VkVertexInputAttributeDescription& attribute = 
      this->_attributeDescriptions.emplace_back();
  attribute.binding = 0;
  attribute.location = attributeId;
  attribute.format = format;
  attribute.offset = offset;

  return attributeId;
}

void GraphicsPipelineBuilder::setPrimitiveType(PrimitiveType type) {
  this->_primitiveType = type;
}

void GraphicsPipelineBuilder::setLineWidth(float width) {
  this->_lineWidth = width;
}

void GraphicsPipelineBuilder::setDepthTesting(bool depthTest) {
  this->_depthTest = depthTest;
}

void GraphicsPipelineBuilder::enableDynamicFrontFace() {
  this->_dynamicStates.push_back(VK_DYNAMIC_STATE_FRONT_FACE);
}
} // namespace AltheaEngine