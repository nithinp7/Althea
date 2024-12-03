#pragma once

#include "Allocator.h"
#include "Library.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace AltheaEngine {
class Application;
class DescriptorSetAllocator;
class DescriptorAssignment;
class TextureHeap;
class BufferHeap;

class ALTHEA_API DescriptorSetLayoutBuilder {
public:
  DescriptorSetLayoutBuilder() {}

  /**
   * @brief Add an acceleration structure binding to the descriptor set layout.
   *
   * @return this builder.
   */
  DescriptorSetLayoutBuilder& addAccelerationStructureBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR);

  DescriptorSetLayoutBuilder& addAccelerationStructureHeapBinding(
      uint32_t count,
      VkShaderStageFlags stageFlags);

  /**
   * @brief Add an image binding to the descriptor set layout for access in a
   * compute shader. Only valid for use in a compute pipeline.
   *
   * @return This builder.
   */
  DescriptorSetLayoutBuilder&
  addStorageImageBinding(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL);
  DescriptorSetLayoutBuilder& addStorageImageHeapBinding(
      uint32_t count,
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL);

  /**
   * @brief Add a storage buffer binding to the descriptor set layout for access
   * in a compute shader. Only valid for use in a compute pipeline.
   *
   * @return This builder.
   */
  DescriptorSetLayoutBuilder& addStorageBufferBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_COMPUTE_BIT);

  /**
   * @brief Add a texture binding to the descriptor set layout.
   *
   * @param stageFlags The shader stages this binding should be accessible to.
   * @return This builder.
   */
  DescriptorSetLayoutBuilder& addTextureBinding(
      VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);

  /**
   * @brief Add a texture heap binding to the descriptor set layout.
   *
   * @param textureCount The number of textures in the heap.
   * @param stageFlags The shader stages this binding should be accesible to.
   * @return This builder.
   */
  DescriptorSetLayoutBuilder&
  addTextureHeapBinding(uint32_t textureCount, VkShaderStageFlags stageFlags);

  /**
   * @brief Add a buffer heap binding to the descriptor set layout.
   *
   * @param bufferCount The number of buffers in the heap.
   * @param stageFlags The shader stages this binding should be accesible to.
   * @return This builder.
   */
  DescriptorSetLayoutBuilder&
  addBufferHeapBinding(uint32_t bufferCount, VkShaderStageFlags stageFlags);

  DescriptorSetLayoutBuilder&
  addUniformHeapBinding(uint32_t bufferCount, VkShaderStageFlags stageFlags);

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

class ALTHEA_API DescriptorSet {
  friend class PerFrameResources; // who cares
public:
  DescriptorSet() = default;
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

  VkDevice _device{};
  VkDescriptorSet _descriptorSet{};

  // TODO: this is ridiculous and not safe, the descriptor set does
  // not work great as a RAII type, it's not worth forcing it this way
  DescriptorSetAllocator* _allocator{};
};

class ALTHEA_API DescriptorAssignment {
public:
  DescriptorAssignment(
      VkDevice device,
      VkDescriptorSet descriptorSet,
      const std::vector<VkDescriptorSetLayoutBinding>& bindings);
  DescriptorAssignment(DescriptorAssignment&& rhs);
  DescriptorAssignment(const DescriptorAssignment& rhs) = delete;
  // DescriptorAssignment& operator=(const DescriptorAssignment& rhs) = delete;

  ~DescriptorAssignment();

  DescriptorAssignment& bindTextureDescriptor(const Texture& texture);

  DescriptorAssignment&
  bindTextureDescriptor(VkImageView imageView, VkSampler sampler);

  DescriptorAssignment&
  bindStorageImage(VkImageView imageView, VkSampler sampler);

  DescriptorAssignment&
  bindAccelerationStructure(VkAccelerationStructureKHR accelerationStructure);

  template <typename TBufferHeap>
  DescriptorAssignment& bindBufferHeap(TBufferHeap& heap) {
    if ((size_t)this->_currentIndex >= this->_bindings.size()) {
      throw std::runtime_error(
          "Exceeded expected number of bindings in descriptor set.");
    }

    if (this->_bindings[this->_currentIndex].descriptorType !=
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
      throw std::runtime_error("Unexpected binding in descriptor set.");
    }

    const std::vector<VkDescriptorBufferInfo>& bufferInfos =
        heap.getBufferInfos();

    VkWriteDescriptorSet& descriptorWrite =
        this->_descriptorWrites[this->_currentIndex];
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->_descriptorSet;
    descriptorWrite.dstBinding = this->_currentIndex;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferInfos.size());
    descriptorWrite.pBufferInfo = bufferInfos.data();
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    ++this->_currentIndex;
    return *this;
  }

  DescriptorAssignment& bindStorageBuffer(
      const BufferAllocation& allocation,
      size_t bufferOffset,
      size_t bufferSize);

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

    this->_descriptorBufferInfos.push_back(new VkDescriptorBufferInfo());
    VkDescriptorBufferInfo& uniformBufferInfo =
        *this->_descriptorBufferInfos.back();
    uniformBufferInfo.buffer = allocation.getBuffer();
    uniformBufferInfo.offset = 0;
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

  DescriptorAssignment&
  bindUniformBufferDescriptor(VkBuffer buffer, size_t offset, size_t size);

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
        new VkWriteDescriptorSetInlineUniformBlock());
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
  std::vector<VkWriteDescriptorSetInlineUniformBlock*> _inlineConstantWrites;
  std::vector<VkDescriptorBufferInfo*> _descriptorBufferInfos;
  std::vector<VkDescriptorImageInfo*> _descriptorImageInfos;
  std::vector<VkWriteDescriptorSetAccelerationStructureKHR*>
      _descriptorAccelerationStructures;
  std::vector<VkAccelerationStructureKHR*> _accelerationStructures;
};

// NOTE: In Vulkan there are no such things as "descriptor set pools", only
// "descriptor pools". For simplicity, we maintain descriptor set pools of
// fixed layouts. Each unique layout will correspond to a unique descriptor
// set allocator, which will block allocate from a descriptor "set" pool
// (a descriptor pool that is only used for one specific layout).

class ALTHEA_API DescriptorSetAllocator {
public:
  DescriptorSetAllocator() = default;

  DescriptorSetAllocator(
      const Application& app,
      const DescriptorSetLayoutBuilder& layoutBuilder,
      uint32_t setsPerPool = 1000);

  DescriptorSet allocate() {
    return DescriptorSet(this->_device, allocateVkDescriptorSet(), *this);
  }

  VkDescriptorSet allocateVkDescriptorSet();
  void free(VkDescriptorSet descriptorSet);

  VkDescriptorSetLayout getLayout() const { return this->_layout; }

  const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const {
    return this->_bindings;
  }

private:
  struct DescriptorSetLayoutDeleter {
    void operator()(VkDevice device, VkDescriptorSetLayout layout);
  };

  struct DescriptorPoolDeleter {
    void operator()(VkDevice device, VkDescriptorPool pool);
  };

  VkDevice _device;

  // Each pool will have this many descriptor sets of the given layout.
  uint32_t _setsPerPool;

  UniqueVkHandle<VkDescriptorSetLayout, DescriptorSetLayoutDeleter> _layout;
  bool _hasInlineUniformBlock = false;

  // Keep this around to check validity when binding resources to a
  // descriptor set.
  std::vector<VkDescriptorSetLayoutBinding> _bindings;

  // When creating a new pool with _setsPerPool count of descriptor sets,
  // _poolSizes will indicate how many descriptors should be allocated for
  // each binding.
  std::vector<VkDescriptorPoolSize> _poolSizes;

  std::vector<UniqueVkHandle<VkDescriptorPool, DescriptorPoolDeleter>> _pools;
  std::vector<VkDescriptorSet> _freeSets;
};
} // namespace AltheaEngine