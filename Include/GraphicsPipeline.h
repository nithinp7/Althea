#pragma once

#include "DescriptorSet.h"
#include "PerFrameResources.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace AltheaEngine {
class Application;
class ShaderManager;

struct PipelineContext {
  VkRenderPass renderPass;
  std::optional<VkDescriptorSetLayout> globalResourcesLayout;
  std::optional<VkDescriptorSetLayout> renderPassResourcesLayout;
  std::optional<VkDescriptorSetLayout> subpassResourcesLayout;
  uint32_t subpassIndex;
};

enum class VertexAttributeType { INT, FLOAT, VEC2, VEC3, VEC4 };

enum class PrimitiveType { TRIANGLES, LINES, POINTS };

class GraphicsPipelineBuilder {
public:
  GraphicsPipelineBuilder&
  addComputeShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder&
  addVertexShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addTessellationControlShader(
      ShaderManager& shaderManager,
      const std::string& shaderPath);
  GraphicsPipelineBuilder& addTessellationEvaluationShader(
      ShaderManager& shaderManager,
      const std::string& shaderPath);
  GraphicsPipelineBuilder& addGeometryShader(
      ShaderManager& shaderManager,
      const std::string& shaderPath);
  GraphicsPipelineBuilder& addFragmentShader(
      ShaderManager& shaderManager,
      const std::string& shaderPath);

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

  /**
   * @brief Set the size of material pools in the material descriptor set block
   * allocator.
   *
   * Small sizes may cause excessive allocation calls as the block count grows.
   * On the other hand, if only a few materials will be needed, it may be better
   * to allocate exactly that many by setting the pool size to the expected
   * material count.
   *
   * @param primitiveCount The max number of primitives that will be rendered
   * with this pipeline.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setMaterialPoolSize(uint32_t poolSize);

  /**
   * @brief Add push constants for this pipeline. Later, push constant ranges
   * can be updated in the command buffer before issuing draw commands.
   *
   * @param stageFlags The shader stages that this push constant range should be
   * made available in.
   * @return This builder.
   */
  template <typename TPushConstants>
  GraphicsPipelineBuilder&
  addPushConstants(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL) {

    uint32_t offset = 0;
    if (!this->_pushConstantRanges.empty()) {
      offset = this->_pushConstantRanges.back().offset;
    }

    VkPushConstantRange& range = this->_pushConstantRanges.emplace_back();
    range.offset = offset;
    range.size = static_cast<uint32_t>(sizeof(TPushConstants));
    range.stageFlags = stageFlags;

    return *this;
  }

  /**
   * @brief Builder for the per-object, material descriptor set layout. The
   * resources bound to descriptor sets of this layout will be unique for each
   * object rendered in this pipeline's subpass.
   */
  DescriptorSetLayoutBuilder materialResourceLayoutBuilder;

private:
  friend class GraphicsPipeline;
  // Info needed to build the graphics pipeline

  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;
  std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;

  std::vector<VkPushConstantRange> _pushConstantRanges;

  VkCullModeFlags _cullMode = VK_CULL_MODE_BACK_BIT;
  VkFrontFace _frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  PrimitiveType _primitiveType = PrimitiveType::TRIANGLES;
  float _lineWidth = 1.0f;

  bool _depthTest = true;

  std::vector<VkDynamicState> _dynamicStates;

  uint32_t _materialPoolSize = 1000;
};

class GraphicsPipeline {
public:
  GraphicsPipeline(
      const Application& app,
      const PipelineContext& context,
      const GraphicsPipelineBuilder& builder);
  GraphicsPipeline(GraphicsPipeline&& rhs) = default;
  ~GraphicsPipeline();

  void bindPipeline(const VkCommandBuffer& commandBuffer) const;

  const VkPipelineLayout& getLayout() const { return this->_pipelineLayout; }

  const VkPipeline& getVkPipeline() const { return this->_pipeline; }

  DescriptorSetAllocator& getMaterialAllocator() {
    return *this->_materialAllocator;
  }

  const std::vector<VkPushConstantRange>& getPushConstantRanges() const {
    return this->_pushConstantRanges;
  }

  bool isDynamicFrontFaceEnabled() const {
    return this->_dynamicFrontFaceEnabled;
  }

private:
  VkDevice _device;

  std::optional<DescriptorSetAllocator> _materialAllocator;

  VkPipelineLayout _pipelineLayout;
  VkPipeline _pipeline;

  // These are kept around to validate client setup of draw calls.
  std::vector<VkPushConstantRange> _pushConstantRanges;
  bool _dynamicFrontFaceEnabled = false;
};
} // namespace AltheaEngine