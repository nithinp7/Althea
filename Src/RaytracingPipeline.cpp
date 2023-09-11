#include "RaytracingPipeline.h"

#include "Application.h"

#include <cstdint>
#include <stdexcept>

namespace AltheaEngine {
// TODO: AnyHit and Intersection shaders...
RayTracingPipeline::RayTracingPipeline(
    const Application& app,
    RayTracingPipelineBuilder&& builder)
    : _pipelineLayout(app, builder.layoutBuilder),
      _rayGenShader(app, builder._rayGenShaderBuilder) {
  uint32_t missCount =
      static_cast<uint32_t>(builder._missShaderBuilders.size());
  uint32_t hitCount =
      static_cast<uint32_t>(builder._closestHitShaderBuilders.size());

  if (missCount == 0 || hitCount == 0) {
    throw std::runtime_error("Cannot create ray tracing pipeline without at "
                             "least one miss shader and one hit shader!");
  }

  this->_missShaders.reserve(missCount);
  for (ShaderBuilder& missBuilder : builder._missShaderBuilders) {
    this->_missShaders.emplace_back(app, missBuilder);
  }

  this->_closestHitShaders.reserve(hitCount);
  for (ShaderBuilder& hitBuilder : builder._closestHitShaderBuilders) {
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
}

void RayTracingPipeline::RayTracingPipelineDeleter::operator()(
    VkDevice device,
    VkPipeline rayTracingPipeline) {
  vkDestroyPipeline(device, rayTracingPipeline, nullptr);
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