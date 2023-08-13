#include "RaytracingPipeline.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
RayTracingPipeline::RayTracingPipeline(Application& app, RayTracingPipelineBuilder&& builder)
  : _pipelineLayout(app, builder.layoutBuilder),
    _rayGenShader(app, builder._rayGenShaderBuilder),
    _anyHitShader(app, builder._anyHitShaderBuilder),
    _intersectionShader(app, builder._intersectionShaderBuilder),
    _closestHitShader(app, builder._closestHitShaderBuilder),
    _missShader(app, builder._missShaderBuilder)
{
  VkPipelineShaderStageCreateInfo shaderStages[5];

  // TODO: Can any of these be optional, look into "default" stages.

  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].module = this->_rayGenShader;
  shaderStages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  shaderStages[0].pName = "main";

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].module = this->_anyHitShader;
  shaderStages[1].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
  shaderStages[1].pName = "main";
  
  shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[2].module = this->_intersectionShader;
  shaderStages[2].stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
  shaderStages[2].pName = "main";
  
  shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[3].module = this->_closestHitShader;
  shaderStages[3].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  shaderStages[3].pName = "main";
  
  shaderStages[4].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[4].module = this->_missShader;
  shaderStages[4].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
  shaderStages[4].pName = "main";

  VkRayTracingPipelineCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  createInfo.layout = this->_pipelineLayout;
  createInfo.maxPipelineRayRecursionDepth = 2;
  createInfo.stageCount = 5;
  createInfo.pStages = shaderStages;
  // createInfo.pDynamicState = ...TODO

  VkDevice device = app.getDevice();
  VkPipeline pipeline;
  if (vkCreateRayTracingPipelinesKHR(
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


void RayTracingPipelineBuilder::setRayGenShader(const std::string& path) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_raygen_shader);
}

void RayTracingPipelineBuilder::setAnyHitShader(const std::string& path) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_anyhit_shader);
}

void RayTracingPipelineBuilder::setIntersectionShader(const std::string& path) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_intersection_shader);
}

void RayTracingPipelineBuilder::setClosestHitShader(const std::string& path) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_closesthit_shader);
}

void RayTracingPipelineBuilder::setMissShader(const std::string& path) {
  this->_rayGenShaderBuilder = ShaderBuilder(path, shaderc_miss_shader);
}

} // namespace AltheaEngine