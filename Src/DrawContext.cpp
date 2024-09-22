#include "DrawContext.h"

#include "RenderPass.h"

#include <cassert>

namespace AltheaEngine {
void DrawContext::bindDescriptorSets(VkDescriptorSet set) const {
  assert(this->_pCurrentSubpass != nullptr);

  uint32_t descriptorSetCount = this->_globalDescriptorSetCount + 1;

  // Mutate _descriptorSets so that all the descriptor sets are
  // contiguous in memory for the binding operation.
  this->_descriptorSets[this->_globalDescriptorSetCount] = set;

  vkCmdBindDescriptorSets(
      this->_commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->_pCurrentSubpass->getPipeline().getLayout(),
      0,
      descriptorSetCount,
      this->_descriptorSets,
      0,
      nullptr);
}

void DrawContext::bindDescriptorSets() const {
  assert(this->_pCurrentSubpass != nullptr);

  uint32_t descriptorSetCount = this->_globalDescriptorSetCount;

  vkCmdBindDescriptorSets(
      this->_commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->_pCurrentSubpass->getPipeline().getLayout(),
      0,
      descriptorSetCount,
      this->_descriptorSets,
      0,
      nullptr);
}

void DrawContext::_updatePushConstants(
    const void* pSrc,
    uint32_t size,
    uint32_t index) const {
  assert(this->_pCurrentSubpass != nullptr);

  const std::vector<VkPushConstantRange>& pushConstantRanges =
      this->_pCurrentSubpass->getPipeline().getPushConstantRanges();

  if (index >= pushConstantRanges.size()) {
    throw std::runtime_error(
        "Invalid push constant range index given during binding.");
  }

  const VkPushConstantRange& range = pushConstantRanges[index];
  if (range.size != size) {
    throw std::runtime_error(
        "Attempting to bind push constant range with unexpected size.");
  }

  vkCmdPushConstants(
      this->_commandBuffer,
      this->_pCurrentSubpass->getPipeline().getLayout(),
      range.stageFlags,
      range.offset,
      range.size,
      pSrc);
}

void DrawContext::setFrontFaceDynamic(VkFrontFace frontFace) const {
  assert(this->_pCurrentSubpass != nullptr);
  assert(this->_pCurrentSubpass->getPipeline().isDynamicFrontFaceEnabled());

  vkCmdSetFrontFace(this->_commandBuffer, frontFace);
}

void DrawContext::setStencilCompareMask(
    VkStencilFaceFlags flags,
    uint32_t cmpMask) const {
  assert(this->_pCurrentSubpass != nullptr);
  assert(this->_pCurrentSubpass->getPipeline().isDynamicStencilEnabled());

  vkCmdSetStencilCompareMask(_commandBuffer, flags, cmpMask);
}

void DrawContext::setStencilWriteMask(
    VkStencilFaceFlags flags,
    uint32_t writeMask) const {
  assert(this->_pCurrentSubpass != nullptr);
  assert(this->_pCurrentSubpass->getPipeline().isDynamicStencilEnabled());

  vkCmdSetStencilWriteMask(_commandBuffer, flags, writeMask);
}
void DrawContext::setStencilReference(
    VkStencilFaceFlags flags,
    uint32_t reference) const {
  assert(this->_pCurrentSubpass != nullptr);
  assert(this->_pCurrentSubpass->getPipeline().isDynamicStencilEnabled());

  vkCmdSetStencilReference(_commandBuffer, flags, reference);
}

void DrawContext::bindIndexBuffer(const IndexBuffer& indexBuffer) const {
  const BufferAllocation& allocation = indexBuffer.getAllocation();
  vkCmdBindIndexBuffer(
      this->_commandBuffer,
      allocation.getBuffer(),
      0,
      VK_INDEX_TYPE_UINT32);
}

void DrawContext::drawIndexed(uint32_t indexCount, uint32_t instanceCount)
    const {
  // TODO: Expose more flexible draw commands, index offset, vertex offset,
  // instance count, etc.
  vkCmdDrawIndexed(this->_commandBuffer, indexCount, instanceCount, 0, 0, 0);
}

void DrawContext::draw(uint32_t vertexCount, uint32_t instanceCount) const {
  vkCmdDraw(this->_commandBuffer, vertexCount, instanceCount, 0, 0);
}
} // namespace AltheaEngine