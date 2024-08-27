#pragma once

#include "DescriptorSet.h"
#include "Library.h"
#include "PerFrameResources.h"
#include "PipelineLayout.h"
#include "Shader.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace AltheaEngine {
class Application;
class ShaderManager;

struct ALTHEA_API PipelineContext {
  VkRenderPass renderPass;
  uint32_t subpassIndex;
  VkExtent2D extent;
  uint32_t colorAttachmentCount;
};

enum class ALTHEA_API VertexAttributeType { UINT, INT, FLOAT, VEC2, VEC3, VEC4 };

enum class ALTHEA_API PrimitiveType { TRIANGLES, LINES, POINTS };

class ALTHEA_API GraphicsPipelineBuilder {
public:
  GraphicsPipelineBuilder& addVertexShader(
      const std::string& shaderPath,
      const ShaderDefines& defines = {});
  GraphicsPipelineBuilder& addTessellationControlShader(
      const std::string& shaderPath,
      const ShaderDefines& defines = {});
  GraphicsPipelineBuilder& addTessellationEvaluationShader(
      const std::string& shaderPath,
      const ShaderDefines& defines = {});
  GraphicsPipelineBuilder& addGeometryShader(
      const std::string& shaderPath,
      const ShaderDefines& defines = {});
  GraphicsPipelineBuilder& addFragmentShader(
      const std::string& shaderPath,
      const ShaderDefines& defines = {});

  /**
   * @brief Add a vertex input binding - can be a vertex buffer or instance
   * buffer. Following calls to addVertexAttribute(...) will be associated with
   * this binding until another call to this function is made to start a new
   * vertex input.
   *
   * @tparam TElementLayout - The structure that will represent the vertex input
   * layout.
   * @return This builder.
   */
  template <typename TElementLayout>
  GraphicsPipelineBuilder& addVertexInputBinding(
      VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX) {
    uint32_t bindingIndex =
        static_cast<uint32_t>(this->_vertexInputBindings.size());
    VkVertexInputBindingDescription& description =
        this->_vertexInputBindings.emplace_back();
    description.binding = bindingIndex;
    description.stride = sizeof(TElementLayout);
    description.inputRate = inputRate;

    return *this;
  }

  /**
   * @brief Add a vertex attribute to the vertex layout of the last-added vertex
   * binding.
   *
   * @param attributeType The type of attribute.
   * @param offset The offset of this attribute within the vertex layout in
   * bytes.
   * @return This builder.
   */
  GraphicsPipelineBuilder&
  addVertexAttribute(VertexAttributeType attributeType, uint32_t offset);

  GraphicsPipelineBuilder&
  addVertexAttribute(VkFormat format, uint32_t offset);

  /**
   * @brief Set the primitive type for this pipeline.
   *
   * @param type The primitive type.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setPrimitiveType(PrimitiveType type);

  /**
   * @brief Set the line width in pixels. Only applies if the primitive
   * type is LINES.
   *
   * @param width The pixel width to render line primitives with.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setLineWidth(float width);


  GraphicsPipelineBuilder& setDepthWrite(bool depthWrite);

  /**
   * @brief Enable or disable depth testing for this pipeline.
   *
   * @param depthTest Whether depth testing should be enabled or disabled.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setDepthTesting(bool depthTest);

  /**
   * @brief Enable dynamically setting the front face for individual objects
   * using this pipeline.
   * @return This builder.
   */
  GraphicsPipelineBuilder& enableDynamicFrontFace();

  /**
   * @brief Set the front face.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setFrontFace(VkFrontFace frontFace);

  /**
   * @brief Set the cull mode.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setCullMode(VkCullModeFlags cullMode);

  // TODO: Create nicer abstraction?
  GraphicsPipelineBuilder& setStencil(const VkStencilOpState& front, const VkStencilOpState& back);
  GraphicsPipelineBuilder& setDynamicStencil(const VkStencilOpState& front, const VkStencilOpState& back);


  /**
   * @brief The builder for creating a pipeline layout for this graphics
   * pipeline.
   */
  PipelineLayoutBuilder layoutBuilder;

private:
  friend class GraphicsPipeline;
  // Info needed to build the graphics pipeline

  // Note: The shader stages do not have shader modules linked at this
  // point, they must be linked when the shader modules are created
  // during construction of the GraphicsPipeline.
  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
  std::vector<ShaderBuilder> _shaderBuilders;

  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;
  std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;

  std::unordered_map<std::string, std::string> _shaderDefines;

  VkCullModeFlags _cullMode = VK_CULL_MODE_BACK_BIT;
  VkFrontFace _frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  PrimitiveType _primitiveType = PrimitiveType::TRIANGLES;
  float _lineWidth = 1.0f;

  bool _depthTest = true;
  bool _depthWrite = true;

  bool _stencilTest = false;
  VkStencilOpState _stencilFront{};
  VkStencilOpState _stencilBack{};

  bool _dynamicStencil = false;

  std::vector<VkDynamicState> _dynamicStates;
};

class ALTHEA_API GraphicsPipeline {
public:
  GraphicsPipeline(
      const Application& app,
      PipelineContext&& context,
      GraphicsPipelineBuilder&& builder);

  void bindPipeline(const VkCommandBuffer& commandBuffer) const;

  VkPipelineLayout getLayout() const { return this->_pipelineLayout; }

  VkPipeline getVkPipeline() const { return this->_pipeline; }

  const std::vector<VkPushConstantRange>& getPushConstantRanges() const {
    return this->_pipelineLayout.getPushConstantRanges();
  }

  bool isDynamicFrontFaceEnabled() const {
    return this->_dynamicFrontFaceEnabled;
  }

  bool isStencilEnabled() const {
    return _builder._stencilTest;
  }

  bool isDynamicStencilEnabled() const {
    return _builder._dynamicStencil;
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
  struct PipelineDeleter {
    void operator()(VkDevice device, VkPipeline pipeline);
  };

  PipelineLayout _pipelineLayout;
  UniqueVkHandle<VkPipeline, PipelineDeleter> _pipeline;

  // These are kept around to allow the pipeline to recreate itself if
  // any shaders become stale.
  PipelineContext _context;
  GraphicsPipelineBuilder _builder;

  // Once the pipeline is recreated, this one will be marked outdated to
  // prevent further pipeline recreations based on this one.
  bool _outdated = false;

  std::vector<Shader> _shaders;

  bool _dynamicFrontFaceEnabled = false;
};
} // namespace AltheaEngine