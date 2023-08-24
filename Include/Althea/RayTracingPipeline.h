#pragma once

#include "Library.h"

#include "PipelineLayout.h"
#include "Shader.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <string> 

namespace AltheaEngine {
class Application;

class ALTHEA_API RayTracingPipelineBuilder {
public:
  void setRayGenShader(const std::string& path);
  void setAnyHitShader(const std::string& path);
  void setIntersectionShader(const std::string& path);
  void setClosestHitShader(const std::string& path);
  void setMissShader(const std::string& path);

  PipelineLayoutBuilder layoutBuilder;

private:
  friend class RayTracingPipeline;

  ShaderBuilder _rayGenShaderBuilder;
  // ShaderBuilder _anyHitShaderBuilder;
  // ShaderBuilder _intersectionShaderBuilder;
  ShaderBuilder _closestHitShaderBuilder;
  ShaderBuilder _missShaderBuilder;
};

class ALTHEA_API RayTracingPipeline {
public:
  RayTracingPipeline(Application& app, RayTracingPipelineBuilder&& builder);

private:
  struct RayTracingPipelineDeleter {
    void operator()(VkDevice device, VkPipeline pipeline);
  };

  PipelineLayout _pipelineLayout;
  UniqueVkHandle<VkPipeline, RayTracingPipelineDeleter> _pipeline;

  Shader _rayGenShader;
  Shader _anyHitShader;
  Shader _intersectionShader;
  Shader _closestHitShader;
  Shader _missShader;
};
} // namespace AltheaEngine

