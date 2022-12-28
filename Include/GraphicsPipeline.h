#pragma once

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
  /**
   * @brief Add a texture binding to the descriptor set layout.
   * 
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  GraphicsPipelineBuilder& addTextureBinding(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
  
  /**
   * @brief Add a uniform buffer binding to the descriptor set layout.
   * 
   * @param stageFlags The shader stages this binding should be accessible to.   
   * @return This builder.
   */
  GraphicsPipelineBuilder& addUniformBufferBinding(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL);
  
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

  // TODO: use actual dynamic descriptor allocator
  /**
   * @brief Set the max number of primitives that will use this pipeline. This will
   * be used to pre-allocate descriptor sets.
   * 
   * @param primitiveCount The max number of primitives that will be rendered with
   * this pipeline.
   * @return This builder.
   */
  GraphicsPipelineBuilder& setPrimitiveCount(uint32_t primitiveCount);
private:
  friend class GraphicsPipeline;
  // Info needed to build the graphics pipeline

  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

  std::vector<VkDescriptorSetLayoutBinding> _descriptorSetLayoutBindings;

  bool _hasBoundConstants = false;
  std::optional<VkDescriptorPoolInlineUniformBlockCreateInfo> _inlineUniformBlockInfo;
  
  std::vector<VkVertexInputBindingDescription> _vertexInputBindings;
  std::vector<VkVertexInputAttributeDescription> _attributeDescriptions;

  VkCullModeFlags _cullMode = VK_CULL_MODE_BACK_BIT;
  VkFrontFace _frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  PrimitiveType _primitiveType = PrimitiveType::TRIANGLES;
  float _lineWidth = 1.0f;

  bool _depthTest = true;

  std::vector<VkDynamicState> _dynamicStates;

  // TODO: descriptor allocator
  uint32_t _primitiveCount;
};

class DescriptorAssignment;

class GraphicsPipeline {
public:
  GraphicsPipeline(
      const Application& app, 
      const PipelineContext& context, 
      const GraphicsPipelineBuilder& builder);
  ~GraphicsPipeline();

  DescriptorAssignment assignDescriptorSet(
      VkDescriptorSet& targetDescriptorSet);
  void returnDescriptorSet(VkDescriptorSet&& freedDescriptorSet);

  void bindPipeline(const VkCommandBuffer& commandBuffer) const;

  const VkPipelineLayout& getLayout() const {
    return this->_pipelineLayout;
  }

  const VkPipeline& getVulkanPipeline() const {
    return this->_pipeline;
  }

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

    this->_descriptorBufferInfos.push_back(std::make_unique<VkDescriptorBufferInfo>());
    VkDescriptorBufferInfo& uniformBufferInfo = 
        *this->_descriptorBufferInfos.back();
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
        
    this->_inlineConstantWrites.push_back(
          std::make_unique<VkWriteDescriptorSetInlineUniformBlock>());
    VkWriteDescriptorSetInlineUniformBlock& inlineConstantsWrite = 
        *this->_inlineConstantWrites.back();
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
  
  // Temporary storage of info needed for descriptor writes
  // TODO: Is there a better way to do this? It looks awkward
  std::vector<std::unique_ptr<VkWriteDescriptorSetInlineUniformBlock>> _inlineConstantWrites;
  std::vector<std::unique_ptr<VkDescriptorBufferInfo>> _descriptorBufferInfos;
  std::vector<std::unique_ptr<VkDescriptorImageInfo>> _descriptorImageInfos;
};
} // namespace AltheaEngine