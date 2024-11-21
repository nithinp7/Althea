#include "GraphicsPipeline.h"

#include "Application.h"
#include "DescriptorSet.h"

#include <algorithm>
#include <iostream>

namespace AltheaEngine {

GraphicsPipeline::GraphicsPipeline(
    const Application& app,
    PipelineContext&& context,
    GraphicsPipelineBuilder&& builder)
    : _pipelineLayout(app, builder.layoutBuilder),
      _context(std::move(context)),
      _builder(std::move(builder)) {

  // Create shader modules
  size_t shaderCount = this->_builder._shaderBuilders.size();
  this->_shaders.reserve(shaderCount);
  for (size_t i = 0; i < shaderCount; ++i) {
    ShaderBuilder& shaderBuilder = this->_builder._shaderBuilders[i];
    this->_shaders.emplace_back(app, shaderBuilder);

    // Link shader module to corresponding shader stage description
    this->_builder._shaderStages[i].module = this->_shaders.back();
  }

  this->_dynamicFrontFaceEnabled =
      std::find(
          this->_builder._dynamicStates.begin(),
          this->_builder._dynamicStates.end(),
          VK_DYNAMIC_STATE_FRONT_FACE) != this->_builder._dynamicStates.end();

  if (this->_builder._shaderStages.empty()) {
    throw std::runtime_error(
        "Attempting to build a graphics pipeline without any shader stages.");
  }

  // VIEWPORT, SCISSOR, ETC

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)context.extent.width;
  viewport.height = (float)context.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = context.extent;

  VkPipelineViewportStateCreateInfo viewportStateInfo{};
  viewportStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateInfo.viewportCount = 1;
  viewportStateInfo.pViewports = &viewport;
  viewportStateInfo.scissorCount = 1;
  viewportStateInfo.pScissors = &scissor;

  // VERTEX INPUT, ATTRIBUTES, ETC

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(this->_builder._vertexInputBindings.size());
  vertexInputInfo.pVertexBindingDescriptions =
      this->_builder._vertexInputBindings.data();
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(this->_builder._attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions =
      this->_builder._attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

  switch (this->_builder._primitiveType) {
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

  // TODO: support transform feedback before rasterizer step
  // (rasterizerDiscardEnable)
  VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
  rasterizerInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerInfo.depthClampEnable = VK_FALSE;
  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;

  switch (this->_builder._primitiveType) {
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

  rasterizerInfo.lineWidth = this->_builder._lineWidth;
  rasterizerInfo.cullMode = this->_builder._cullMode;
  rasterizerInfo.frontFace = this->_builder._frontFace;
  rasterizerInfo.depthBiasEnable = VK_FALSE;
  rasterizerInfo.depthBiasConstantFactor = 0.0f;
  rasterizerInfo.depthBiasClamp = 0.0f;
  rasterizerInfo.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
  multisamplingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingInfo.sampleShadingEnable = VK_FALSE;
  multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisamplingInfo.minSampleShading = 1.0f;
  multisamplingInfo.pSampleMask = nullptr;
  multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
  multisamplingInfo.alphaToOneEnable = VK_FALSE;

  // TODO: can this be generalized in a useful way?
  // ... might need to pass in more detailed info about attachments
  // through PipelineContext
  // Make alpha blending optional?
  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
  colorBlendAttachments.resize(context.colorAttachmentCount);
  for (uint32_t i = 0; i < context.colorAttachmentCount; ++i) {
    VkPipelineColorBlendAttachmentState& colorBlendAttachment =
        colorBlendAttachments[i];
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  }

  VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
  colorBlendingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingInfo.logicOpEnable = VK_FALSE;
  colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlendingInfo.attachmentCount = context.colorAttachmentCount;
  colorBlendingInfo.pAttachments = colorBlendAttachments.data();
  colorBlendingInfo.blendConstants[0] = 0.0f; // Optional
  colorBlendingInfo.blendConstants[1] = 0.0f; // Optional
  colorBlendingInfo.blendConstants[2] = 0.0f; // Optional
  colorBlendingInfo.blendConstants[3] = 0.0f; // Optional

  // DEPTH STENCIL

  // Only gets used if this->_depthTest is true
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
  depthStencilInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilInfo.depthTestEnable = VK_TRUE;
  depthStencilInfo.depthWriteEnable = builder._depthWrite;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.minDepthBounds = 0.0f; // Not used
  depthStencilInfo.maxDepthBounds = 1.0f; // Not used
  depthStencilInfo.stencilTestEnable = builder._stencilTest;
  depthStencilInfo.front = builder._stencilFront;
  depthStencilInfo.back = builder._stencilBack;

  // DYNAMIC STATES

  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(this->_builder._dynamicStates.size());
  dynamicStateInfo.pDynamicStates = this->_builder._dynamicStates.data();

  // CREATE THE GRAPHICS PIPELINE

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount =
      static_cast<uint32_t>(this->_builder._shaderStages.size());
  pipelineInfo.pStages = this->_builder._shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  pipelineInfo.pViewportState = &viewportStateInfo;
  pipelineInfo.pRasterizationState = &rasterizerInfo;
  pipelineInfo.pMultisampleState = &multisamplingInfo;
  pipelineInfo.pDepthStencilState =
      this->_builder._depthTest ? &depthStencilInfo : nullptr;
  pipelineInfo.pColorBlendState = &colorBlendingInfo;
  pipelineInfo.pDynamicState = &dynamicStateInfo;
  pipelineInfo.layout = this->_pipelineLayout;
  pipelineInfo.renderPass = this->_context.renderPass;
  pipelineInfo.subpass = this->_context.subpassIndex;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  VkDevice device = app.getDevice();
  VkPipeline pipeline;
  if (vkCreateGraphicsPipelines(
          device,
          VK_NULL_HANDLE,
          1,
          &pipelineInfo,
          nullptr,
          &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }

  this->_pipeline.set(device, pipeline);
}

void GraphicsPipeline::PipelineDeleter::operator()(
    VkDevice device,
    VkPipeline pipeline) {
  vkDestroyPipeline(device, pipeline, nullptr);
}

void GraphicsPipeline::bindPipeline(
    const VkCommandBuffer& commandBuffer) const {
  vkCmdBindPipeline(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->_pipeline);
}

void GraphicsPipeline::tryRecompile(Application& app) {
  if (recompileStaleShaders()) {
    if (hasShaderRecompileErrors()) {
      std::cout << getShaderRecompileErrors() << "\n" << std::flush;
    } else {
      recreatePipeline(app);
    }
  }
}

bool GraphicsPipeline::recompileStaleShaders() {
  if (this->_outdated) {
    return false;
  }

  bool stale = false;
  for (ShaderBuilder& shader : this->_builder._shaderBuilders) {
    if (shader.reloadIfStale()) {
      stale = true;
      shader.recompile();
    }
  }

  return stale;
}

bool GraphicsPipeline::hasShaderRecompileErrors() const {
  if (this->_outdated) {
    // This is not necessarily a shader recompile error...
    // but we don't want to recreate the pipeline if it has already
    // been recreated from.
    return true;
  }

  for (const ShaderBuilder& shader : this->_builder._shaderBuilders) {
    if (shader.hasErrors()) {
      return true;
    }
  }

  return false;
}

std::string GraphicsPipeline::getShaderRecompileErrors() const {
  if (this->_outdated) {
    return "";
  }

  std::string errors;
  for (const ShaderBuilder& shader : this->_builder._shaderBuilders) {
    if (shader.hasErrors()) {
      errors += shader.getErrors() + "\n";
    }
  }

  return errors;
}

void GraphicsPipeline::recreatePipeline(Application& app) {
  // Mark this pipeline as outdated so we don't recreate another pipeline
  // from it.
  // TODO: This may be a silly constraint, the only reason for it is that we
  // are moving away the context and builder objects when recreating the
  // pipeline...
  this->_outdated = true;

  GraphicsPipeline newPipeline(
      app,
      std::move(this->_context),
      std::move(this->_builder));

  // Since they will now have invalid values.
  this->_context = {};
  this->_builder = {};

  GraphicsPipeline* pOldPipeline = new GraphicsPipeline(std::move(*this));
  app.addDeletiontask(std::move(DeletionTask{
      [pOldPipeline]() { delete pOldPipeline; },
      app.getCurrentFrameRingBufferIndex()}));

  *this = std::move(newPipeline);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexShader(
    const std::string& shaderPath,
    const ShaderDefines& defines) {
  this->_shaderBuilders.emplace_back(
      shaderPath,
      shaderc_vertex_shader,
      defines);

  VkPipelineShaderStageCreateInfo& vertShaderStageInfo =
      this->_shaderStages.emplace_back();
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = VK_NULL_HANDLE; // Will get linked later.
  vertShaderStageInfo.pName = "main";

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addTessellationControlShader(
    const std::string& shaderPath,
    const ShaderDefines& defines) {
  throw std::runtime_error("Tessellation shaders not yet supported!");

  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::addTessellationEvaluationShader(
    const std::string& shaderPath,
    const ShaderDefines& defines) {
  throw std::runtime_error("Tessellation shaders not yet supported!");

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addGeometryShader(
    const std::string& shaderPath,
    const ShaderDefines& defines) {
  throw std::runtime_error("Geometry shaders not yet supported!");
  // VkPipelineShaderStageCreateInfo& geomShaderStageInfo =
  //    this->_shaderStages.emplace_back();
  // geomShaderStageInfo.sType =
  // VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  // geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
  // geomShaderStageInfo.module = VK_NULL_HANDLE; // Will get linked later.
  // geomShaderStageInfo.pName = "main";

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addFragmentShader(
    const std::string& shaderPath,
    const ShaderDefines& defines) {
  this->_shaderBuilders.emplace_back(
      shaderPath,
      shaderc_fragment_shader,
      defines);

  VkPipelineShaderStageCreateInfo& fragShaderStageInfo =
      this->_shaderStages.emplace_back();
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = VK_NULL_HANDLE; // Will get linked later.
  fragShaderStageInfo.pName = "main";

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::addVertexAttribute(
    VertexAttributeType attributeType,
    uint32_t offset) {

  VkFormat format;
  switch (attributeType) {
  case VertexAttributeType::UINT:
    format = VK_FORMAT_R32_UINT;
    break;
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

  return addVertexAttribute(format, offset);
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::addVertexAttribute(VkFormat format, uint32_t offset) {

  // Check that this attribute corresponds to an actual vertex input binding.
  if (this->_vertexInputBindings.empty()) {
    throw std::runtime_error("Attempting to add vertex attribute without any "
                             "vertex input bindings.");
  }
  uint32_t attributeId =
      static_cast<uint32_t>(this->_attributeDescriptions.size());
  VkVertexInputAttributeDescription& attribute =
      this->_attributeDescriptions.emplace_back();
  attribute.binding =
      static_cast<uint32_t>(this->_vertexInputBindings.size() - 1);
  attribute.location = attributeId;
  attribute.format = format;
  attribute.offset = offset;

  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setPrimitiveType(PrimitiveType type) {
  this->_primitiveType = type;

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setLineWidth(float width) {
  this->_lineWidth = width;

  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setDepthWrite(bool depthWrite) {
  this->_depthWrite = depthWrite;

  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setDepthTesting(bool depthTest) {
  this->_depthTest = depthTest;

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enableDynamicFrontFace() {
  this->_dynamicStates.push_back(VK_DYNAMIC_STATE_FRONT_FACE);

  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setFrontFace(VkFrontFace frontFace) {
  this->_frontFace = frontFace;

  return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::setCullMode(VkCullModeFlags cullMode) {
  this->_cullMode = cullMode;

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setStencil(
  const VkStencilOpState& front,
  const VkStencilOpState& back) {
  _stencilTest = true;
  _stencilFront = front;
  _stencilBack = back;

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDynamicStencil(
    const VkStencilOpState& front,
    const VkStencilOpState& back) {
  if (!_dynamicStencil) {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    _dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    _dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
  }

  _dynamicStencil = true;
  _stencilTest = true;
  _stencilFront = front;
  _stencilBack = back;

  return *this;
}

std::string GraphicsPipelineBuilder::compileShadersGetErrors()
{
  std::string errors;
  for (ShaderBuilder& shader : _shaderBuilders) {
    if (!shader.recompile())
    {
      errors += shader.getErrors() + "\n";
    }
  }
  return errors;
}
} // namespace AltheaEngine