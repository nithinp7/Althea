#pragma once

#include "DescriptorSet.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <cstdint>
#include <string>
#include <stdexcept>

#include <memory>

namespace AltheaEngine {
class Application;
class ShaderManager;

struct PipelineContext {
  VkRenderPass renderPass;
  uint32_t subpassIndex;
};

enum class VertexAttributeType {
  INT,
  FLOAT,
  VEC2,
  VEC3,
  VEC4
};

enum class PrimitiveType {
  TRIANGLES,
  LINES,
  POINTS
};

class GraphicsPipelineBuilder {
public:
  GraphicsPipelineBuilder& addComputeShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addVertexShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addTessellationControlShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addTessellationEvaluationShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addGeometryShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addFragmentShader(ShaderManager& shaderManager, const std::string& shaderPath);

  /**
   * @brief Add a vertex input binding - can be a vertex buffer or instance buffer. Following calls to
   * addVertexAttribute(...) will be associated with this binding until another call to this function
   * is made to start a new vertex input.
   * 
   * @tparam TElementLayout - The structure that will represent the vertex input layout.
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
   * @brief Add a vertex attribute to the vertex layout of the last-added vertex binding.
   * 
   * @param attributeType The type of attribute.
   * @param offset The offset of this attribute within the vertex layout in bytes.
   * @return This builder.
   */
  GraphicsPipelineBuilder& addVertexAttribute(VertexAttributeType attributeType, uint32_t offset);

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
   * @param primitiveCount The max number of primitives that will be rendered with
   * this pipeline.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setMaterialPoolSize(uint32_t poolSize);

  /**
   * @brief Set the descriptor set for global resources. These will be made available
   * in all shaders.
   * 
   * @param layout The resource layout of this descriptor set.
   * @param descriptorSet The global descriptor set. 
   * @return This builder.
   */
  GraphicsPipelineBuilder& setGlobalResources(
      VkDescriptorSetLayout layout,
      const std::shared_ptr<DescriptorSet>& pDescriptorSet);

  /** TODO: Are these cases needed, or can we just do material vs global resoures?
   * 
   * @brief Set the descriptor set for render pass-wide resources. Such resources
   * will be made available in all shaders within this render pass.
   * 
   * @param layout The resoure layout of this descriptor set.
   * @param descriptorSet The render pass wide descriptor set.
   * @return This builder. 
   */
  GraphicsPipelineBuilder& setRenderPassResources(
      VkDescriptorSetLayout layout,
      const std::shared_ptr<DescriptorSet>& pDescriptorSet);

  /**
   * @brief Builder for the subpass-wide descriptor set layout. The resources bound
   * to the descriptor set with this layout will be available in shaders across all 
   * objects rendered in this pipeline's subpass.
   */
  DescriptorSetLayoutBuilder subpassResourceLayoutBuilder;

  /**
   * @brief Builder for the per-object, material descriptor set layout. The resources
   * bound to descriptor sets of this layout will be unique for each object rendered
   * in this pipeline's subpass. 
   */
  DescriptorSetLayoutBuilder materialResourceLayoutBuilder;

private:
  friend class GraphicsPipeline;
  // Info needed to build the graphics pipeline

  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
  
  std::optional<VkDescriptorSetLayout> _globalResourceLayout;
  std::shared_ptr<DescriptorSet> _pGlobalResources;

  std::optional<VkDescriptorSetLayout> _renderPassResourceLayout;
  std::shared_ptr<DescriptorSet> _pRenderPassResources;

  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;
  std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;

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

  const VkPipelineLayout& getLayout() const {
    return this->_pipelineLayout;
  }

  const VkPipeline& getVkPipeline() const {
    return this->_pipeline;
  }

  DescriptorAssignment beginBindSubpassResources();

  DescriptorSetAllocator& getMaterialAllocator() {
    return *this->_materialAllocator;
  }

  const DescriptorSet* getGlobalResources() const {
    return this->_pGlobalResources.get();
  }

  const DescriptorSet* getRenderPassResources() const {
    return this->_pRenderPassResources.get();
  }

private:
  VkDevice _device;

  std::shared_ptr<DescriptorSet> _pGlobalResources;
  std::shared_ptr<DescriptorSet> _pRenderPassResources;

  std::optional<DescriptorSetAllocator> _subpassResourcesAllocator;
  std::optional<DescriptorSet> _subpassResources;

  std::optional<DescriptorSetAllocator> _materialAllocator;

  VkPipelineLayout _pipelineLayout;
  VkPipeline _pipeline;
};
} // namespace AltheaEngine