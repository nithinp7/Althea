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
   * @param defines Defines to inject before compilation
   */
  void
  setComputeShader(const std::string& path, const ShaderDefines& defines = {});


  std::string compileShadersGetErrors();

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
  void
  bindDescriptorSet(VkCommandBuffer commandBuffer, VkDescriptorSet set) const;
  void bindDescriptorSets(
      VkCommandBuffer commandBuffer,
      const VkDescriptorSet* sets,
      uint32_t count) const;

  template <typename TPush>
  void
  setPushConstants(VkCommandBuffer commandBuffer, const TPush& push) const {
    vkCmdPushConstants(
        commandBuffer,
        getLayout(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(TPush),
        &push);
  }

  VkPipeline getPipeline() const { return this->_pipeline; }

  VkPipelineLayout getLayout() const { return this->_pipelineLayout; }

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
  struct ComputePipelineDeleter {
    void operator()(VkDevice device, VkPipeline pipeline);
  };

  PipelineLayout _pipelineLayout;
  UniqueVkHandle<VkPipeline, ComputePipelineDeleter> _pipeline;

  Shader _shader;

  ComputePipelineBuilder _builder;

  // Once the pipeline is recreated, this one will be marked outdated to
  // prevent further pipeline recreations based on this one.
  bool _outdated = false;
};
} // namespace AltheaEngine