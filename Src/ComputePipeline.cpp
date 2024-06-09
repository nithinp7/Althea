#include "ComputePipeline.h"

#include "Application.h"

#include <iostream>
#include <stdexcept>

namespace AltheaEngine {
void ComputePipelineBuilder::setComputeShader(
    const std::string& path,
    const ShaderDefines& defines) {
  this->_shaderBuilder = ShaderBuilder(path, shaderc_compute_shader, defines);
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

void ComputePipeline::bindDescriptorSet(
    VkCommandBuffer commandBuffer,
    VkDescriptorSet set) const {
  bindDescriptorSets(commandBuffer, &set, 1);
}

void ComputePipeline::bindDescriptorSets(
    VkCommandBuffer commandBuffer,
    const VkDescriptorSet* sets,
    uint32_t count) const {
  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      getLayout(),
      0,
      count,
      sets,
      0,
      nullptr);
}

void ComputePipeline::ComputePipelineDeleter::operator()(
    VkDevice device,
    VkPipeline computePipeline) {
  vkDestroyPipeline(device, computePipeline, nullptr);
}

void ComputePipeline::tryRecompile(Application& app) {
  if (recompileStaleShaders()) {
    if (hasShaderRecompileErrors()) {
      std::cout << getShaderRecompileErrors() << "\n" << std::flush;
    } else {
      recreatePipeline(app);
    }
  }
}

bool ComputePipeline::recompileStaleShaders() {
  if (this->_outdated) {
    return false;
  }

  bool stale = false;
  if (this->_builder._shaderBuilder.reloadIfStale()) {
    stale = true;
    this->_builder._shaderBuilder.recompile();
  }

  return stale;
}

bool ComputePipeline::hasShaderRecompileErrors() const {
  if (this->_outdated) {
    // This is not necessarily a shader recompile error...
    // but we don't want to recreate the pipeline if it has already
    // been recreated from.
    return true;
  }

  return this->_builder._shaderBuilder.hasErrors();
}

std::string ComputePipeline::getShaderRecompileErrors() const {
  if (!this->_outdated && this->_builder._shaderBuilder.hasErrors()) {
    return this->_builder._shaderBuilder.getErrors() + "\n";
  }

  return "";
}

void ComputePipeline::recreatePipeline(Application& app) {
  // Mark this pipeline as outdated so we don't recreate another pipeline
  // from it.
  // TODO: This may be a silly constraint, the only reason for it is that we
  // are moving away the context and builder objects when recreating the
  // pipeline...
  this->_outdated = true;

  ComputePipeline newPipeline(app, std::move(this->_builder));

  // Since they will now have invalid values.
  this->_builder = {};

  ComputePipeline* pOldPipeline = new ComputePipeline(std::move(*this));
  app.addDeletiontask(std::move(DeletionTask{
      [pOldPipeline]() { delete pOldPipeline; },
      app.getCurrentFrameRingBufferIndex()}));

  *this = std::move(newPipeline);
}
} // namespace AltheaEngine