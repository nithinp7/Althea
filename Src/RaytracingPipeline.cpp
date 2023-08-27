#include "RaytracingPipeline.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
RayTracingPipeline::RayTracingPipeline(
    Application& app,
    RayTracingPipelineBuilder&& builder)
    : _pipelineLayout(app, builder.layoutBuilder),
      _rayGenShader(app, builder._rayGenShaderBuilder),
      //_anyHitShader(app, builder._anyHitShaderBuilder),
      //_intersectionShader(app, builder._intersectionShaderBuilder),
      _closestHitShader(app, builder._closestHitShaderBuilder),
      _missShader(app, builder._missShaderBuilder) {
  VkPipelineShaderStageCreateInfo shaderStages[5] = {};
  VkRayTracingShaderGroupCreateInfoKHR shaderGroups[5] = {};
  // TODO: Can any of these be optional, look into "default" stages.

  // RAY GEN
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].module = this->_rayGenShader;
  shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  shaderStages[0].pName = "main";

  shaderGroups[0].sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shaderGroups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  shaderGroups[0].generalShader = 0;
  shaderGroups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
  shaderGroups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
  shaderGroups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

  // MISS
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].module = this->_missShader;
  shaderStages[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
  shaderStages[1].pName = "main";

  shaderGroups[1].sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shaderGroups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  shaderGroups[1].generalShader = 1;
  shaderGroups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
  shaderGroups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
  shaderGroups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

  // CLOSEST HIT
  shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[2].module = this->_closestHitShader;
  shaderStages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  shaderStages[2].pName = "main";

  shaderGroups[2].sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shaderGroups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  shaderGroups[2].generalShader = VK_SHADER_UNUSED_KHR;
  shaderGroups[2].closestHitShader = 2;
  shaderGroups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
  shaderGroups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

  // TODO: shader stages and groups for option shaders
  // TODO: Properly make these two optional
  // shaderStages[3].sType =
  // VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; shaderStages[3].module
  // = this->_anyHitShader; shaderStages[3].stage =
  // VK_SHADER_STAGE_ANY_HIT_BIT_KHR; shaderStages[3].pName = "main";

  // shaderStages[4].sType =
  // VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; shaderStages[4].module
  // = this->_intersectionShader; shaderStages[4].stage =
  // VK_SHADER_STAGE_INTERSECTION_BIT_KHR; shaderStages[4].pName = "main";

  VkRayTracingPipelineCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  createInfo.layout = this->_pipelineLayout;
  createInfo.maxPipelineRayRecursionDepth = 1;
  createInfo.stageCount = 3; // TODO: This should be 5 if
  createInfo.pStages = shaderStages;
  createInfo.groupCount = 3;
  createInfo.pGroups = shaderGroups;
  // createInfo.pDynamicState = ...TODO

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

void RayTracingPipelineBuilder::setRayGenShader(const std::string& path) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_raygen_shader);
}

void RayTracingPipelineBuilder::setAnyHitShader(const std::string& path) {
  // this->_anyHiShaderBuilder = ShaderBuilder(path, shaderc_anyhit_shader);
}

void RayTracingPipelineBuilder::setIntersectionShader(const std::string& path) {
  // this->_intersectionShaderBuilder = ShaderBuilder(path,
  // shaderc_intersection_shader);
}

void RayTracingPipelineBuilder::setClosestHitShader(const std::string& path) {
  this->_closestHitShaderBuilder =
      ShaderBuilder(path, shaderc_closesthit_shader);
}

void RayTracingPipelineBuilder::setMissShader(const std::string& path) {
  this->_missShaderBuilder = ShaderBuilder(path, shaderc_miss_shader);
}

} // namespace AltheaEngine