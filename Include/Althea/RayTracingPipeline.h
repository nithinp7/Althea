#pragma once

#include "Library.h"

#include "PipelineLayout.h"
#include "Shader.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <string> 
#include <vector>

namespace AltheaEngine {
class Application;

class ALTHEA_API RayTracingPipelineBuilder {
public:
  void setRayGenShader(const std::string& path, const ShaderDefines& defs = {});
  void addMissShader(const std::string& path, const ShaderDefines& defs = {});
  void addClosestHitShader(const std::string& path, const ShaderDefines& defs = {});

  PipelineLayoutBuilder layoutBuilder;

private:
  friend class RayTracingPipeline;

  ShaderBuilder _rayGenShaderBuilder;
  std::vector<ShaderBuilder> _missShaderBuilders;
  std::vector<ShaderBuilder> _closestHitShaderBuilders;

  // TODO: Anyhit, intersection
};

class ALTHEA_API RayTracingPipeline {
public:
  RayTracingPipeline(const Application& app, RayTracingPipelineBuilder&& builder);

  VkPipelineLayout getLayout() const {
    return this->_pipelineLayout;
  }
  
  operator VkPipeline() const {
    return this->_pipeline;
  }

  uint32_t getMissShaderCount() const {
    return static_cast<uint32_t>(this->_missShaders.size());
  }

  uint32_t getClosestHitShaderCount() const {
    return static_cast<uint32_t>(this->_closestHitShaders.size());
  }

private:
  struct RayTracingPipelineDeleter {
    void operator()(VkDevice device, VkPipeline pipeline);
  };

  PipelineLayout _pipelineLayout;
  UniqueVkHandle<VkPipeline, RayTracingPipelineDeleter> _pipeline;

  Shader _rayGenShader;
  std::vector<Shader> _missShaders;
  std::vector<Shader> _closestHitShaders;

  // TODO: Anyhit, intersection
};
} // namespace AltheaEngine

