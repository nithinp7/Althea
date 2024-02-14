#include "RaytracingPipeline.h"

#include "Application.h"

#include <cstdint>
#include <stdexcept>

#include <iostream>

namespace AltheaEngine {
// TODO: AnyHit and Intersection shaders...
RayTracingPipeline::RayTracingPipeline(
    const Application& app,
    RayTracingPipelineBuilder&& builder)
    : _builder(std::move(builder)),
      _pipelineLayout(app, _builder.layoutBuilder),
      _rayGenShader(app, _builder._rayGenShaderBuilder) {
  uint32_t missCount =
      static_cast<uint32_t>(_builder._missShaderBuilders.size());
  uint32_t hitCount =
      static_cast<uint32_t>(_builder._closestHitShaderBuilders.size());

  if (missCount == 0 || hitCount == 0) {
    throw std::runtime_error("Cannot create ray tracing pipeline without at "
                             "least one miss shader and one hit shader!");
  }

  this->_missShaders.reserve(missCount);
  for (ShaderBuilder& missBuilder : _builder._missShaderBuilders) {
    this->_missShaders.emplace_back(app, missBuilder);
  }

  this->_closestHitShaders.reserve(hitCount);
  for (ShaderBuilder& hitBuilder : _builder._closestHitShaderBuilders) {
    this->_closestHitShaders.emplace_back(app, hitBuilder);
  }

  uint32_t shaderCount = 1 + missCount + hitCount;

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.reserve(shaderCount);
  std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
  shaderStages.reserve(shaderCount);

  // TODO: Can any of these be optional, look into "default" stages.

  uint32_t shaderIdx = 0;

  // RAY GEN
  {
    VkPipelineShaderStageCreateInfo& stage = shaderStages.emplace_back();
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = this->_rayGenShader;
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stage.pName = "main";

    VkRayTracingShaderGroupCreateInfoKHR& group = shaderGroups.emplace_back();
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = shaderIdx++;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
  }

  // MISS
  for (const Shader& missShader : this->_missShaders) {
    VkPipelineShaderStageCreateInfo& stage = shaderStages.emplace_back();
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = missShader;
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stage.pName = "main";

    VkRayTracingShaderGroupCreateInfoKHR& group = shaderGroups.emplace_back();
    group.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = shaderIdx++;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
  }

  // CLOSEST HIT
  for (const Shader& closestHitShader : this->_closestHitShaders) {
    VkPipelineShaderStageCreateInfo& stage = shaderStages.emplace_back();
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = closestHitShader;
    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stage.pName = "main";

    VkRayTracingShaderGroupCreateInfoKHR& group = shaderGroups.emplace_back();
    group.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type =
        VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = shaderIdx++;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
  }

  VkRayTracingPipelineCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  createInfo.layout = this->_pipelineLayout;
  createInfo.maxPipelineRayRecursionDepth = 1;
  createInfo.stageCount = shaderCount;
  createInfo.pStages = shaderStages.data();
  createInfo.groupCount = shaderCount;
  createInfo.pGroups = shaderGroups.data();

  VkDevice device = app.getDevice();
  VkPipeline pipeline;
  if (app.vkCreateRayTracingPipelinesKHR(
          device,
          VK_NULL_HANDLE,
          VK_NULL_HANDLE,
          1,
          &createInfo,
          nullptr,
          &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create ray tracing pipeline!");
  }

  this->_pipeline.set(device, pipeline);

  this->_sbt = ShaderBindingTable(app, *this);
}

void RayTracingPipeline::RayTracingPipelineDeleter::operator()(
    VkDevice device,
    VkPipeline rayTracingPipeline) {
  vkDestroyPipeline(device, rayTracingPipeline, nullptr);
}

void RayTracingPipeline::tryRecompile(Application &app) {  
  if (recompileStaleShaders()) {
    if (hasShaderRecompileErrors()) {
      std::cout << getShaderRecompileErrors() << "\n" << std::flush;
    } else {
      recreatePipeline(app);
    }
  }
}

bool RayTracingPipeline::recompileStaleShaders() {
  if (this->_outdated) {
    return false;
  }

  bool stale = false;
  if (this->_builder._rayGenShaderBuilder.reloadIfStale()) {
    stale = true;
    this->_builder._rayGenShaderBuilder.recompile();
  }

  for (ShaderBuilder& shader : this->_builder._closestHitShaderBuilders) {
    if (shader.reloadIfStale()) {
      stale = true;
      shader.recompile();
    }
  }

  for (ShaderBuilder& shader : this->_builder._missShaderBuilders) {
    if (shader.reloadIfStale()) {
      stale = true;
      shader.recompile();
    }
  }

  return stale;
}

bool RayTracingPipeline::hasShaderRecompileErrors() const {
  if (this->_outdated) {
    // This is not necessarily a shader recompile error...
    // but we don't want to recreate the pipeline if it has already
    // been recreated from.
    return true;
  }

  if (this->_builder._rayGenShaderBuilder.hasErrors()) {
    return true;
  }

  for (const ShaderBuilder& shader : this->_builder._closestHitShaderBuilders) {
    if (shader.hasErrors()) {
      return true;
    }
  }

  for (const ShaderBuilder& shader : this->_builder._missShaderBuilders) {
    if (shader.hasErrors()) {
      return true;
    }
  }

  return false;
}

std::string RayTracingPipeline::getShaderRecompileErrors() const {
  if (this->_outdated) {
    return "";
  }

  std::string errors;
  if (this->_builder._rayGenShaderBuilder.hasErrors()) {
    errors += this->_builder._rayGenShaderBuilder.getErrors() + "\n";
  }

  for (const ShaderBuilder& shader : this->_builder._closestHitShaderBuilders) {
    if (shader.hasErrors()) {
      errors += shader.getErrors() + "\n";
    }
  }

  for (const ShaderBuilder& shader : this->_builder._missShaderBuilders) {
    if (shader.hasErrors()) {
      errors += shader.getErrors() + "\n";
    }
  }

  return errors;
}

void RayTracingPipeline::traceRays(const VkExtent2D& extent, VkCommandBuffer commandBuffer) const {
  Application::vkCmdTraceRaysKHR(
      commandBuffer,
      &this->_sbt.getRayGenRegion(),
      &this->_sbt.getMissRegion(),
      &this->_sbt.getHitRegion(),
      &this->_sbt.getCallRegion(),
      extent.width,
      extent.height,
      1);
}

void RayTracingPipeline::recreatePipeline(Application& app) {
  // Mark this pipeline as outdated so we don't recreate another pipeline
  // from it.
  // TODO: This may be a silly constraint, the only reason for it is that we
  // are moving away the context and builder objects when recreating the
  // pipeline...
  this->_outdated = true;

  RayTracingPipeline newPipeline(
      app,
      std::move(this->_builder));

  // Since they will now have invalid values.
  this->_builder = {};

  RayTracingPipeline* pOldPipeline = new RayTracingPipeline(std::move(*this));
  app.addDeletiontask(std::move(DeletionTask{
      [pOldPipeline]() { delete pOldPipeline; },
      app.getCurrentFrameRingBufferIndex()}));

  *this = std::move(newPipeline);
}

void RayTracingPipelineBuilder::setRayGenShader(const std::string& path, const ShaderDefines& defs) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_raygen_shader, defs);
}

void RayTracingPipelineBuilder::addMissShader(const std::string& path, const ShaderDefines& defs) {
  this->_missShaderBuilders.emplace_back(path, shaderc_miss_shader, defs);
}

void RayTracingPipelineBuilder::addClosestHitShader(const std::string& path, const ShaderDefines& defs) {
  this->_closestHitShaderBuilders.emplace_back(path, shaderc_closesthit_shader, defs);
}
} // namespace AltheaEngine