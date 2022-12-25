#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace AltheaEngine {
class Application;
class ShaderManager;

struct PipelineContext {
  VkRenderPass renderPass;
  uint32_t subpassIndex;
  uint32_t primitiveCount;
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
  /**
   * @brief Add a texture binding to the descriptor set layout.
   * 
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  GraphicsPipelineBuilder& addTextureBinding(VkShaderStageFlags stageFlags);
  
  /**
   * @brief Add a uniform buffer binding to the descriptor set layout.
   * 
   * @param stageFlags The shader stages this binding should be accessible to.   
   * @return This builder.
   */
  GraphicsPipelineBuilder& addUniformBufferBinding(VkShaderStageFlags stageFlags);
  
  /**
   * @brief Add a constant, inline uniform buffer to the descriptor set layout.
   * 
   * @tparam TPrimitiveConstants - The structure that will be used to represent the
   * inline constant uniform buffer.
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  template <typename TPrimitiveConstants>
  GraphicsPipelineBuilder& addConstantsBufferBinding(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL) {
    uint32_t bindingIndex = 
        static_cast<uint32_t>(this->_descriptorSetLayoutBindings.size());
    VkDescriptorSetLayoutBinding& binding = 
        this->_descriptorSetLayoutBindings.emplace_back();
    binding.binding = bindingIndex;
    binding.descriptorCount = sizeof(TPrimitiveConstants);
    binding.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = stageFlags;

    this->_hasBoundConstants = true;

    return *this;
  }

  GraphicsPipelineBuilder& addComputeShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addVertexShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addTessellationControlShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addTessellationEvaluationShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addGeometryShader(ShaderManager& shaderManager, const std::string& shaderPath);
  GraphicsPipelineBuilder& addFragmentShader(ShaderManager& shaderManager, const std::string& shaderPath);

  /**
   * @brief Set the vertex buffer binding.
   * 
   * @tparam TVertex - The structure that will represent the vertex layout.
   * @return This builder.
   */
  template <typename TVertex>
  GraphicsPipelineBuilder& setVertexBufferBinding() {
    this->_bindingDescription.binding = 0;
    this->_bindingDescription.stride = sizeof(TVertex);
    this->_bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return *this;
  }

  /**
   * @brief Add a vertex attribute to the vertex layout.
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
private:
  friend class GraphicsPipeline;
  // Info needed to build the graphics pipeline

  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

  std::vector<VkDescriptorSetLayoutBinding> _descriptorSetLayoutBindings;

  bool _hasBoundConstants = false;
  std::optional<VkDescriptorPoolInlineUniformBlockCreateInfo> _inlineUniformBlockInfo;
  
  VkVertexInputBindingDescription _bindingDescription;
  std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;

  PrimitiveType _primitiveType = PrimitiveType::TRIANGLES;
  float _lineWidth = 1.0f;

  bool _depthTest = true;

  std::vector<VkDynamicState> _dynamicStates;
};

class DescriptorAssignment;

class GraphicsPipeline {
public:
  GraphicsPipeline(
      const Application& app, 
      const PipelineContext& context, 
      const GraphicsPipelineBuilder& builder);
  ~GraphicsPipeline();

  // TODO:
  DescriptorAssignment assignDescriptorSet(
      VkDescriptorSet& targetDescriptorSet);
  void returnDescriptorSet(VkDescriptorSet&& freedDescriptorSet);
  void bindPipeline(const VkCommandBuffer& commandBuffer);

private:
  friend class DescriptorAssignment;

  VkDevice _device;

  // Keep this around to check validity when binding resources to a 
  // descriptor set.
  std::vector<VkDescriptorSetLayoutBinding> _descriptorSetLayoutBindings;

  // TODO: dynamic descriptor allocator, currently all descriptors need
  // to be created up front.
  std::vector<VkDescriptorSet> _descriptorSets;

  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;

  VkPipelineLayout _pipelineLayout;
  VkPipeline _pipeline;
};

class DescriptorAssignment {
public:
  DescriptorAssignment(
      GraphicsPipeline& pipeline, 
      VkDescriptorSet& targetDescriptorSet);
  ~DescriptorAssignment();

  DescriptorAssignment& bindTextureDescriptor(
      VkImageView imageView, VkSampler sampler);

  template <typename TUniforms>
  DescriptorAssignment& bindUniformBufferDescriptor(VkBuffer uniformBuffer) {
    if ((size_t)this->_currentIndex >= 
      this->_pipeline._descriptorSetLayoutBindings.size()) {
      throw std::runtime_error("Exceeded expected number of bindings in descriptor set.");
    }

    if (this->_pipeline._descriptorSetLayoutBindings[this->_currentIndex].descriptorType !=
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      throw std::runtime_error("Unexpected binding in descriptor set.");
    }

    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(TUniforms);

    VkWriteDescriptorSet& descriptorWrite = 
        this->_descriptorWrites[this->_currentIndex];
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->_targetDescriptorSet;
    descriptorWrite.dstBinding = this->_currentIndex;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &uniformBufferInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    ++this->_currentIndex;
    return *this;
  }

  template <typename TPrimitiveConstants>
  DescriptorAssignment& bindInlineConstantDescriptors(
      const TPrimitiveConstants* pConstants) {
    if ((size_t)this->_currentIndex >= 
        this->_pipeline._descriptorSetLayoutBindings.size()) {
      throw std::runtime_error("Exceeded expected number of bindings in descriptor set.");
    }

    if (this->_pipeline._descriptorSetLayoutBindings[this->_currentIndex].descriptorType !=
        VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      throw std::runtime_error("Unexpected binding in descriptor set.");
    }

    VkWriteDescriptorSetInlineUniformBlock inlineConstantsWrite{};
    inlineConstantsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK;
    inlineConstantsWrite.dataSize = sizeof(TPrimitiveConstants);
    inlineConstantsWrite.pData = pConstants;

    VkWriteDescriptorSet& descriptorWrite = 
        this->_descriptorWrites[this->_currentIndex];
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->_targetDescriptorSet;
    descriptorWrite.dstBinding = this->_currentIndex;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
    descriptorWrite.descriptorCount = sizeof(TPrimitiveConstants);
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;
    descriptorWrite.pNext = &inlineConstantsWrite;

    ++this->_currentIndex;
    return *this;
  }
  
private:
  uint32_t _currentIndex = 0;
  GraphicsPipeline& _pipeline;
  VkDescriptorSet& _targetDescriptorSet;

  std::vector<VkWriteDescriptorSet> _descriptorWrites;
};
} // namespace AltheaEngine