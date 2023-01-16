#include "ComputePipeline.h"

#include "Application.h"

namespace AltheaEngine {
ComputePipeline::ComputePipeline(
    const Application& app,
    ComputePipelineBuilder&& builder)
    : _pipelineLayout(app, builder.layoutBuilder),
      _shader(app, builder.shaderBuilder),
      _builder(std::move(builder)) {
  // vkCreateComputePipelines pipelineInfo{};
  // pipelineInfo
}

void ComputePipeline::ComputePipelineDeleter::operator()(
    VkDevice device,
    VkPipeline computePipeline) {
  vkDestroyPipeline(device, computePipeline, nullptr);
}
} // namespace AltheaEngine