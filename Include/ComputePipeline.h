#pragma once

#include "Library.h"

#include "PipelineLayout.h"
#include "Shader.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;
class ComputePipeline;

class ALTHEA_API ComputePipelineBuilder {
public:
  /**
   * @brief Set the compute shader for this compute pipeline.
   *
   * @param path The project-relative path to the glsl compute shader.
   */
  void setComputeShader(const std::string& path);

  /**
   * @brief The builder for creating a pipeline layout for this graphics
   * pipeline.
   */
  PipelineLayoutBuilder layoutBuilder;

private:
  friend class ComputePipeline;

  ShaderBuilder _shaderBuilder;
};

class ALTHEA_API ComputePipeline {
public:
  ComputePipeline() = default;
  ComputePipeline(const Application& app, ComputePipelineBuilder&& builder);

  void bindPipeline(VkCommandBuffer commandBuffer) const;

  VkPipeline getPipeline() const { return this->_pipeline; }

  VkPipelineLayout getLayout() const { return this->_pipelineLayout; }

private:
  struct ComputePipelineDeleter {
    void operator()(VkDevice device, VkPipeline pipeline);
  };

  PipelineLayout _pipelineLayout;
  UniqueVkHandle<VkPipeline, ComputePipelineDeleter> _pipeline;

  Shader _shader;

  ComputePipelineBuilder _builder;
};
} // namespace AltheaEngine