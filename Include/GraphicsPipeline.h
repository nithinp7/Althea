#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <cstdint>
#include <string>

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
   * @return The binding index.
   */
  uint32_t addTextureBinding(VkShaderStageFlags stageFlags);
  
  /**
   * @brief Add a uniform buffer binding to the descriptor set layout.
   * 
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return The binding index.
   */
  uint32_t addUniformBufferBinding(VkShaderStageFlags stageFlags);
  
  /**
   * @brief Add a constant, inline uniform buffer to the descriptor set layout.
   * 
   * @tparam TPrimitiveConstants - The structure that will be used to represent the
   * inline constant uniform buffer.
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return The binding index.
   */
  template <typename TPrimitiveConstants>
  uint32_t addConstantsBufferBinding(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL) {
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

    return bindingIndex;
  }

  void addComputeShader(ShaderManager& shaderManager, const std::string& shaderPath);
  void addVertexShader(ShaderManager& shaderManager, const std::string& shaderPath);
  void addTessellationControlShader(ShaderManager& shaderManager, const std::string& shaderPath);
  void addTessellationEvaluationShader(ShaderManager& shaderManager, const std::string& shaderPath);
  void addGeometryShader(ShaderManager& shaderManager, const std::string& shaderPath);
  void addFragmentShader(ShaderManager& shaderManager, const std::string& shaderPath);

  /**
   * @brief Set the vertex buffer binding.
   * 
   * @tparam TVertex - The structure that will represent the vertex layout.
   */
  template <typename TVertex>
  void setVertexBufferBinding() {
    this->_bindingDescription.binding = 0;
    this->_bindingDescription.stride = sizeof(TVertex);
    this->_bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  }

  /**
   * @brief Add a vertex attribute to the vertex layout.
   * 
   * @param attributeType The type of attribute.
   * @param offset The offset of this attribute within the vertex layout in bytes.
   * @return The location index of this attribute within the vertex layout.
   */
  uint32_t addVertexAttribute(VertexAttributeType attributeType, uint32_t offset);

  /**
   * @brief Set the primitive type for this pipeline.
   * 
   * @param type The primitive type.
   */
  void setPrimitiveType(PrimitiveType type); 

  /**
   * @brief Set the line width in pixels. Only applies if the primitive 
   * type is LINES.
   * 
   * @param width The pixel width to render line primitives with.
   */
  void setLineWidth(float width);

  /**
   * @brief Enable or disable depth testing for this pipeline.
   * 
   * @param depthTest Whether depth testing should be enabled or disabled. 
   */
  void setDepthTesting(bool depthTest);

  /**
   * @brief Enable dynamically setting the front face for individual objects
   * using this pipeline. 
   */
  void enableDynamicFrontFace();
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

class GraphicsPipeline {
public:
  GraphicsPipeline(
      const Application& app, 
      const PipelineContext& context, 
      const GraphicsPipelineBuilder& builder);
  ~GraphicsPipeline();

private:
  VkDevice _device;

  // TODO: dynamic descriptor allocator, currently all descriptors need
  // to be created up front.
  std::vector<VkDescriptorSet> _descriptorSets;

  // TODO: do these belong in the pipeline class?
  VkDescriptorSetLayout _descriptorSetLayout;
  VkDescriptorPool _descriptorPool;

  VkPipelineLayout _pipelineLayout;
  VkPipeline _pipeline;
};
} // namespace AltheaEngine