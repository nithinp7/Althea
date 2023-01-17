#include "ComputePipeline.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
void ComputePipelineBuilder::setComputeShader(const std::string& path) {
  this->_shaderBuilder = ShaderBuilder(path, shaderc_compute_shader);
}

ComputePipeline::ComputePipeline(
    const Application& app,
    ComputePipelineBuilder&& builder)
    : _pipelineLayout(app, builder.layoutBuilder),
      _shader(app, builder._shaderBuilder),
      _builder(std::move(builder)) {
  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = this->_pipelineLayout;

  pipelineInfo.stage.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineInfo.stage.module = this->_shader;
  pipelineInfo.stage.pName = "main";

  VkDevice device = app.getDevice();
  VkPipeline computePipeline;
  if (vkCreateComputePipelines(
          device,
          VK_NULL_HANDLE,
          1,
          &pipelineInfo,
          nullptr,
          &computePipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create compute pipeline!");
  }

  this->_pipeline.set(device, computePipeline);
}

void ComputePipeline::bindPipeline(VkCommandBuffer commandBuffer) const {
  vkCmdBindPipeline(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      this->_pipeline);
}

void ComputePipeline::ComputePipelineDeleter::operator()(
    VkDevice device,
    VkPipeline computePipeline) {
  vkDestroyPipeline(device, computePipeline, nullptr);
}
} // namespace AltheaEngine