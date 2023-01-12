#pragma once

#include "Texture.h"
#include "UniformBuffer.h"
#include "Allocator.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;
class DescriptorSetAllocator;
class DescriptorAssignment;

class DescriptorSetLayoutBuilder {
public:
  DescriptorSetLayoutBuilder() {}

  /**
   * @brief Add a texture binding to the descriptor set layout.
   *
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  DescriptorSetLayoutBuilder& addTextureBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);

  /**
   * @brief Add a uniform buffer binding to the descriptor set layout.
   *
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  DescriptorSetLayoutBuilder&
  addUniformBufferBinding(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL);

  /**
   * @brief Add a constant, inline uniform buffer to the descriptor set layout.
   *
   * @tparam TPrimitiveConstants - The structure that will be used to represent
   * the inline constant uniform buffer.
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  template <typename TPrimitiveConstants>
  DescriptorSetLayoutBuilder& addConstantsBufferBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL) {
    uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
    VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
    binding.binding = bindingIndex;
    binding.descriptorCount = sizeof(TPrimitiveConstants);
    binding.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = stageFlags;

    this->_hasInlineUniformBlock = true;

    return *this;
  }

  bool hasBindings() const { return !this->_bindings.empty(); }

private:
  friend class DescriptorSetAllocator;

  bool _hasInlineUniformBlock = false;
  std::vector<VkDescriptorSetLayoutBinding> _bindings{};
};

class DescriptorSet {
public:
  DescriptorSet(DescriptorSet&& rhs) noexcept;
  DescriptorSet& operator=(DescriptorSet&& rhs) noexcept;

  ~DescriptorSet();

  const VkDescriptorSet& getVkDescriptorSet() const {
    return this->_descriptorSet;
  }

  DescriptorAssignment assign();

private:
  friend class DescriptorSetAllocator;

  DescriptorSet(
      VkDevice device,
      VkDescriptorSet descriptorSet,
      DescriptorSetAllocator& allocator);

  VkDevice _device;
  VkDescriptorSet _descriptorSet;

  DescriptorSetAllocator& _allocator;
};

class DescriptorAssignment {
public:
  DescriptorAssignment(
      VkDevice device,
      VkDescriptorSet descriptorSet,
      const std::vector<VkDescriptorSetLayoutBinding>& bindings);
  DescriptorAssignment(DescriptorAssignment&& rhs);
  DescriptorAssignment(const DescriptorAssignment& rhs) = delete;
  // DescriptorAssignment& operator=(const DescriptorAssignment& rhs) = delete;

  ~DescriptorAssignment();

  DescriptorAssignment&
  bindTextureDescriptor(const Texture& texture);

  DescriptorAssignment&
  bindTextureDescriptor(VkImageView imageView, VkSampler sampler);

  template <typename TUniforms>
  DescriptorAssignment&
  bindUniformBufferDescriptor(const UniformBuffer<TUniforms>& ubo) {
    if ((size_t)this->_currentIndex >= this->_bindings.size()) {
      throw std::runtime_error(
          "Exceeded expected number of bindings in descriptor set.");
    }

    if (this->_bindings[this->_currentIndex].descriptorType !=
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      throw std::runtime_error("Unexpected binding in descriptor set.");
    }

    const BufferAllocation& allocation = ubo.getAllocation();

    this->_descriptorBufferInfos.push_back(
        std::make_unique<VkDescriptorBufferInfo>());
    VkDescriptorBufferInfo& uniformBufferInfo =
        *this->_descriptorBufferInfos.back();
    uniformBufferInfo.buffer = allocation.getBuffer();
    uniformBufferInfo.offset = allocation.getInfo().offset;
    uniformBufferInfo.range = ubo.getSize();

    VkWriteDescriptorSet& descriptorWrite =
        this->_descriptorWrites[this->_currentIndex];
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->_descriptorSet;
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
  DescriptorAssignment&
  bindInlineConstantDescriptors(const TPrimitiveConstants& constants) {
    if ((size_t)this->_currentIndex >= this->_bindings.size()) {
      throw std::runtime_error(
          "Exceeded expected number of bindings in descriptor set.");
    }

    if (this->_bindings[this->_currentIndex].descriptorType !=
        VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      throw std::runtime_error("Unexpected binding in descriptor set.");
    }

    this->_inlineConstantWrites.push_back(
        std::make_unique<VkWriteDescriptorSetInlineUniformBlock>());
    VkWriteDescriptorSetInlineUniformBlock& inlineConstantsWrite =
        *this->_inlineConstantWrites.back();
    inlineConstantsWrite.sType =
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK;
    inlineConstantsWrite.dataSize = sizeof(TPrimitiveConstants);
    inlineConstantsWrite.pData = &constants;

    VkWriteDescriptorSet& descriptorWrite =
        this->_descriptorWrites[this->_currentIndex];
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->_descriptorSet;
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

  VkDevice _device;
  VkDescriptorSet _descriptorSet;
  const std::vector<VkDescriptorSetLayoutBinding>& _bindings;

  std::vector<VkWriteDescriptorSet> _descriptorWrites;

  // Temporary storage of info needed for descriptor writes
  // TODO: Is there a better way to do this? It looks awkward
  std::vector<std::unique_ptr<VkWriteDescriptorSetInlineUniformBlock>>
      _inlineConstantWrites;
  std::vector<std::unique_ptr<VkDescriptorBufferInfo>> _descriptorBufferInfos;
  std::vector<std::unique_ptr<VkDescriptorImageInfo>> _descriptorImageInfos;
};

// NOTE: In Vulkan there are no such things as "descriptor set pools", only
// "descriptor pools". For simplicity, we maintain descriptor set pools of
// fixed layouts. Each unique layout will correspond to a unique descriptor
// set allocator, which will block allocate from a descriptor "set" pool
// (a descriptor pool that is only used for one specific layout).

class DescriptorSetAllocator {
public:
  DescriptorSetAllocator(
      const Application& app,
      const DescriptorSetLayoutBuilder& layoutBuilder,
      uint32_t setsPerPool = 1000);
  ~DescriptorSetAllocator();

  DescriptorSet allocate();
  void free(VkDescriptorSet descriptorSet);

  VkDescriptorSetLayout getLayout() const { return this->_layout; }

  const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const {
    return this->_bindings;
  }

private:
  VkDevice _device;

  // Each pool will have this many descriptor sets of the given layout.
  uint32_t _setsPerPool;

  VkDescriptorSetLayout _layout;
  bool _hasInlineUniformBlock = false;

  // Keep this around to check validity when binding resources to a
  // descriptor set.
  std::vector<VkDescriptorSetLayoutBinding> _bindings;

  // When creating a new pool with _setsPerPool count of descriptor sets,
  // _poolSizes will indicate how many descriptors should be allocated for
  // each binding.
  std::vector<VkDescriptorPoolSize> _poolSizes;

  std::vector<VkDescriptorPool> _pools;
  std::vector<VkDescriptorSet> _freeSets;
};
} // namespace AltheaEngine