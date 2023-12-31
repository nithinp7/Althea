#include "DescriptorSet.h"

#include "Application.h"
#include "BufferHeap.h"
#include "TextureHeap.h"

#include <cassert>
#include <stdexcept>

namespace AltheaEngine {
DescriptorSetLayoutBuilder&
DescriptorSetLayoutBuilder::addAccelerationStructureBinding(
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addStorageImageBinding(
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addStorageBufferBinding(
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder&
DescriptorSetLayoutBuilder::addTextureBinding(VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addTextureHeapBinding(
    uint32_t textureCount,
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = textureCount;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addBufferHeapBinding(
    uint32_t bufferCount,
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = bufferCount;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addUniformHeapBinding(
    uint32_t bufferCount,
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = bufferCount;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addUniformBufferBinding(
    VkShaderStageFlags stageFlags) {
  uint32_t bindingIndex = static_cast<uint32_t>(this->_bindings.size());
  VkDescriptorSetLayoutBinding& binding = this->_bindings.emplace_back();
  binding.binding = bindingIndex;
  binding.descriptorCount = 1;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  binding.pImmutableSamplers = nullptr;
  binding.stageFlags = stageFlags;

  return *this;
}

DescriptorSet::DescriptorSet(DescriptorSet&& rhs) noexcept
    : _device(rhs._device),
      _descriptorSet(rhs._descriptorSet),
      _allocator(rhs._allocator) {
  // Is this hacky? This prevents the moved-from object from
  // releasing the descriptor set handle.
  rhs._descriptorSet = VK_NULL_HANDLE;
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& rhs) noexcept {
  if (this->_descriptorSet != VK_NULL_HANDLE && this->_allocator) {
    this->_allocator->free(this->_descriptorSet);
  }

  this->_device = rhs._device;
  this->_descriptorSet = rhs._descriptorSet;
  this->_allocator = rhs._allocator;

  rhs._descriptorSet = VK_NULL_HANDLE;
  rhs._allocator = nullptr;

  return *this;
}

DescriptorSet::DescriptorSet(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    DescriptorSetAllocator& allocator)
    : _device(device), _descriptorSet(descriptorSet), _allocator(&allocator) {}

DescriptorSet::~DescriptorSet() {
  if (this->_descriptorSet != VK_NULL_HANDLE && this->_allocator) {
    this->_allocator->free(this->_descriptorSet);
  }
}

DescriptorAssignment DescriptorSet::assign() {
  return DescriptorAssignment(
      this->_device,
      this->_descriptorSet,
      this->_allocator->getBindings());
}

DescriptorAssignment::DescriptorAssignment(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    : _device(device), _descriptorSet(descriptorSet), _bindings(bindings) {
  this->_descriptorWrites.resize(this->_bindings.size());
}

DescriptorAssignment::DescriptorAssignment(DescriptorAssignment&& rhs)
    : _currentIndex(rhs._currentIndex),
      _device(rhs._device),
      _descriptorSet(rhs._descriptorSet),
      _bindings(rhs._bindings),
      _descriptorWrites(std::move(rhs._descriptorWrites)),
      _inlineConstantWrites(std::move(rhs._inlineConstantWrites)),
      _descriptorBufferInfos(std::move(rhs._descriptorBufferInfos)),
      _descriptorImageInfos(std::move(rhs._descriptorImageInfos)) {
  rhs._descriptorWrites.clear();
}

template <typename T> static void releaseTempData(std::vector<T*>& data) {
  for (T* pT : data)
    delete pT;
  data.clear();
}

// TODO: Use an explicit endBinding function instead of the destructor
// since we cannot throw or assert in destructors.
DescriptorAssignment::~DescriptorAssignment() {
  // The descriptor writes for this descriptor set are commited when this
  // assignment object goes out of scope.
  if (!this->_descriptorWrites.empty()) {
    vkUpdateDescriptorSets(
        this->_device,
        static_cast<uint32_t>(this->_descriptorWrites.size()),
        this->_descriptorWrites.data(),
        0,
        nullptr);
  }

  releaseTempData(this->_inlineConstantWrites);
  releaseTempData(this->_descriptorBufferInfos);
  releaseTempData(this->_descriptorImageInfos);
  releaseTempData(this->_descriptorAccelerationStructures);
  releaseTempData(this->_accelerationStructures);
}

DescriptorAssignment&
DescriptorAssignment::bindTextureDescriptor(const Texture& texture) {
  return this->bindTextureDescriptor(
      texture.getImageView(),
      texture.getSampler());
}

DescriptorAssignment& DescriptorAssignment::bindTextureDescriptor(
    VkImageView imageView,
    VkSampler sampler) {
  if ((size_t)this->_currentIndex >= this->_bindings.size()) {
    throw std::runtime_error(
        "Exceeded expected number of bindings in descriptor set.");
  }

  if (this->_bindings[this->_currentIndex].descriptorType !=
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
    throw std::runtime_error("Unexpected binding in descriptor set.");
  }

  this->_descriptorImageInfos.push_back(new VkDescriptorImageInfo());
  VkDescriptorImageInfo& textureImageInfo = *this->_descriptorImageInfos.back();

  textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureImageInfo.imageView = imageView;
  textureImageInfo.sampler = sampler;

  VkWriteDescriptorSet& descriptorWrite =
      this->_descriptorWrites[this->_currentIndex];
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = this->_descriptorSet;
  descriptorWrite.dstBinding = this->_currentIndex;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pImageInfo = &textureImageInfo;
  descriptorWrite.pTexelBufferView = nullptr;

  ++this->_currentIndex;
  return *this;
}

DescriptorAssignment& DescriptorAssignment::bindTextureHeap(TextureHeap& heap) {
  if ((size_t)this->_currentIndex >= this->_bindings.size()) {
    throw std::runtime_error(
        "Exceeded expected number of bindings in descriptor set.");
  }

  if (this->_bindings[this->_currentIndex].descriptorType !=
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
    throw std::runtime_error("Unexpected binding in descriptor set.");
  }

  const std::vector<VkDescriptorImageInfo>& imageInfos = heap.getImageInfos();

  VkWriteDescriptorSet& descriptorWrite =
      this->_descriptorWrites[this->_currentIndex];
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = this->_descriptorSet;
  descriptorWrite.dstBinding = this->_currentIndex;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pImageInfo = imageInfos.data();
  descriptorWrite.pTexelBufferView = nullptr;

  ++this->_currentIndex;
  return *this;
}

DescriptorAssignment& DescriptorAssignment::bindStorageImage(
    VkImageView imageView,
    VkSampler sampler) {
  if ((size_t)this->_currentIndex >= this->_bindings.size()) {
    throw std::runtime_error(
        "Exceeded expected number of bindings in descriptor set.");
  }

  if (this->_bindings[this->_currentIndex].descriptorType !=
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
    throw std::runtime_error("Unexpected binding in descriptor set.");
  }

  this->_descriptorImageInfos.push_back(new VkDescriptorImageInfo());
  VkDescriptorImageInfo& textureImageInfo = *this->_descriptorImageInfos.back();

  textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  textureImageInfo.imageView = imageView;
  textureImageInfo.sampler = sampler;

  VkWriteDescriptorSet& descriptorWrite =
      this->_descriptorWrites[this->_currentIndex];
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = this->_descriptorSet;
  descriptorWrite.dstBinding = this->_currentIndex;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pImageInfo = &textureImageInfo;
  descriptorWrite.pTexelBufferView = nullptr;

  ++this->_currentIndex;
  return *this;
}

DescriptorAssignment& DescriptorAssignment::bindAccelerationStructure(
    VkAccelerationStructureKHR accelerationStructure) {
  if ((size_t)this->_currentIndex >= this->_bindings.size()) {
    throw std::runtime_error(
        "Exceeded expected number of bindings in descriptor set.");
  }

  if (this->_bindings[this->_currentIndex].descriptorType !=
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
    throw std::runtime_error("Unexpected binding in descriptor set.");
  }

  this->_accelerationStructures.push_back(
      new VkAccelerationStructureKHR(accelerationStructure));

  this->_descriptorAccelerationStructures.push_back(
      new VkWriteDescriptorSetAccelerationStructureKHR());

  VkWriteDescriptorSetAccelerationStructureKHR& accelerationStructureInfo =
      *this->_descriptorAccelerationStructures.back();

  accelerationStructureInfo.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  accelerationStructureInfo.accelerationStructureCount = 1;
  accelerationStructureInfo.pAccelerationStructures =
      this->_accelerationStructures.back();

  VkWriteDescriptorSet& descriptorWrite =
      this->_descriptorWrites[this->_currentIndex];
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = this->_descriptorSet;
  descriptorWrite.dstBinding = this->_currentIndex;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pImageInfo = nullptr;
  descriptorWrite.pTexelBufferView = nullptr;
  descriptorWrite.pNext = &accelerationStructureInfo;

  ++this->_currentIndex;
  return *this;
}

DescriptorAssignment& DescriptorAssignment::bindStorageBuffer(
    const BufferAllocation& allocation,
    size_t bufferOffset,
    size_t bufferSize) {

  this->_descriptorBufferInfos.push_back(new VkDescriptorBufferInfo());
  VkDescriptorBufferInfo& bufferInfo = *this->_descriptorBufferInfos.back();
  bufferInfo.buffer = allocation.getBuffer();
  bufferInfo.offset = bufferOffset;
  bufferInfo.range = bufferSize;

  VkWriteDescriptorSet& descriptorWrite =
      this->_descriptorWrites[this->_currentIndex];
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = this->_descriptorSet;
  descriptorWrite.dstBinding = this->_currentIndex;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;
  descriptorWrite.pImageInfo = nullptr;
  descriptorWrite.pTexelBufferView = nullptr;

  ++this->_currentIndex;
  return *this;
}

DescriptorSetAllocator::DescriptorSetAllocator(
    const Application& app,
    const DescriptorSetLayoutBuilder& layoutBuilder,
    uint32_t setsPerPool)
    : _device(app.getDevice()),
      _setsPerPool(setsPerPool),
      _hasInlineUniformBlock(layoutBuilder._hasInlineUniformBlock),
      _bindings(layoutBuilder._bindings) {

  // This is stupid
  std::vector<VkDescriptorBindingFlags> bindingFlags{};
  bindingFlags.resize(layoutBuilder._bindings.size());
  for (auto& flag : bindingFlags) {
    flag |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
  }

  VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
  bindingFlagsInfo.bindingCount = layoutBuilder._bindings.size();
  bindingFlagsInfo.pBindingFlags = bindingFlags.data();
  bindingFlagsInfo.pNext = nullptr;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount =
      static_cast<uint32_t>(layoutBuilder._bindings.size());
  descriptorSetLayoutInfo.pBindings = layoutBuilder._bindings.data();
  descriptorSetLayoutInfo.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
  descriptorSetLayoutInfo.pNext = &bindingFlagsInfo;

  VkDescriptorSetLayout layout;
  if (vkCreateDescriptorSetLayout(
          this->_device,
          &descriptorSetLayoutInfo,
          nullptr,
          &layout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }

  this->_layout.set(this->_device, layout);

  // Create descriptor pool sizes according to the descriptor set layout.
  // This will be used later when creating new pools.
  this->_poolSizes.reserve(layoutBuilder._bindings.size());
  for (const VkDescriptorSetLayoutBinding& binding : layoutBuilder._bindings) {
    VkDescriptorPoolSize& poolSize = this->_poolSizes.emplace_back();
    poolSize.type = binding.descriptorType;
    poolSize.descriptorCount = this->_setsPerPool * binding.descriptorCount;
  }
}

void DescriptorSetAllocator::DescriptorSetLayoutDeleter::operator()(
    VkDevice device,
    VkDescriptorSetLayout layout) {
  vkDestroyDescriptorSetLayout(device, layout, nullptr);
}

void DescriptorSetAllocator::DescriptorPoolDeleter::operator()(
    VkDevice device,
    VkDescriptorPool pool) {
  vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet DescriptorSetAllocator::allocateVkDescriptorSet() {
  if (this->_freeSets.empty()) {
    // No free sets available, create a new descriptor set pool.
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(this->_poolSizes.size());
    poolInfo.pPoolSizes = this->_poolSizes.data();
    poolInfo.maxSets = this->_setsPerPool;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VkDescriptorPoolInlineUniformBlockCreateInfo
        inlineDescriptorPoolCreateInfo{};
    if (this->_hasInlineUniformBlock) {
      inlineDescriptorPoolCreateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO;
      inlineDescriptorPoolCreateInfo.maxInlineUniformBlockBindings =
          this->_setsPerPool;

      poolInfo.pNext = &inlineDescriptorPoolCreateInfo;
    }

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(this->_device, &poolInfo, nullptr, &pool) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create descriptor pool!");
    }

    this->_pools.emplace_back(this->_device, pool);

    // Allocate all the descriptor sets for the new pool.
    std::vector<VkDescriptorSetLayout> layouts(
        this->_setsPerPool,
        this->_layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = this->_setsPerPool;
    allocInfo.pSetLayouts = layouts.data();

    size_t currentFreeSetsCount = this->_freeSets.size();
    this->_freeSets.resize(currentFreeSetsCount + this->_setsPerPool);
    if (vkAllocateDescriptorSets(
            this->_device,
            &allocInfo,
            &this->_freeSets[currentFreeSetsCount]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor sets!");
    }
  }

  assert(!this->_freeSets.empty());

  VkDescriptorSet allocatedSet = this->_freeSets.back();
  this->_freeSets.pop_back();

  return allocatedSet;
}

void DescriptorSetAllocator::free(VkDescriptorSet set) {
  this->_freeSets.push_back(set);
}
} // namespace AltheaEngine