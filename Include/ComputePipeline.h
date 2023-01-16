#pragma once

#include "PipelineLayout.h"
#include "Shader.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

namespace AltheaEngine {
class Application;
class ComputePipeline;

struct ComputePipelineBuilder {
  /**
   * @brief The builder for creating a pipeline layout for this graphics
   * pipeline.
   */
  PipelineLayoutBuilder layoutBuilder;

  /**
   * @brief The builder for this pipeline's compute shader.
   */
  ShaderBuilder shaderBuilder;
};

class ComputePipeline {
public:
  ComputePipeline(const Application& app, ComputePipelineBuilder&& builder);

  void bindPipeline() const;

  VkPipeline getPipeline() const { return this->_pipeline; }

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