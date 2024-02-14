#pragma once

#include "Library.h"

#include "PipelineLayout.h"
#include "Shader.h"
#include "UniqueVkHandle.h"
#include "ShaderBindingTable.h"

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
  RayTracingPipeline() = default;
  RayTracingPipeline(const Application& app, RayTracingPipelineBuilder&& builder);

  void traceRays(const VkExtent2D& extent, VkCommandBuffer commandBuffer) const;

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

  void tryRecompile(Application& app);
  /**
   * @brief Reload and attempt to recompile any stale shaders that is
   * out-of-date with the corresponding shader file on disk. Note that the
   * recompile is not guaranteed to have succeeded, check
   * hasShaderRecompileErrors() before recreating the pipeline.
   *
   *
   * @return Whether any stale shaders were detected and reloaded /
   * recompiled.
   */
  bool recompileStaleShaders();

  /**
   * @brief Whether any reloaded shaders have errors during recompilation. Note
   * that it is NOT safe to recreate the pipeline if there are any shader
   * recompile errors remaining.
   *
   * @return Whether there were any errors when recompiling reloaded shaders.
   */
  bool hasShaderRecompileErrors() const;

  /**
   * @brief Get compilation error messages that were created while reloading
   * and recompiling stale shaders.
   *
   * @return The shader compilation errors.
   */
  std::string getShaderRecompileErrors() const;

  /**
   * @brief Create a new pipeline with all the same paramaters as the current
   * one, but uses the newly recompiled shaders if they exist. Note this
   * function is NOT safe to call if any of the recompiled shaders have
   * compilation errors - check that hasShaderRecompileErrors() is false first.
   *
   * @return The new pipeline based on the current one, but with updated and
   * recompiled shaders if they exist.
   */
  void recreatePipeline(Application& app);

private:
  struct RayTracingPipelineDeleter {
    void operator()(VkDevice device, VkPipeline pipeline);
  };

  RayTracingPipelineBuilder _builder;

  PipelineLayout _pipelineLayout;
  UniqueVkHandle<VkPipeline, RayTracingPipelineDeleter> _pipeline;

  ShaderBindingTable _sbt;;

  Shader _rayGenShader;
  std::vector<Shader> _missShaders;
  std::vector<Shader> _closestHitShaders;

  bool _outdated = false;
  // TODO: Anyhit, intersection
};
} // namespace AltheaEngine

