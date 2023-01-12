#include "DrawContext.h"

#include "RenderPass.h"

#include <cassert>

namespace AltheaEngine {
void DrawContext::bindDescriptorSets(const Material& material) const {
  assert(this->_pCurrentSubpass != nullptr);

  VkDescriptorSet descriptorSets[4];

  uint32_t descriptorSetCount = 0;
  if (this->_globalResources) {
    descriptorSets[descriptorSetCount] = *this->_globalResources;
    ++descriptorSetCount;
  }

  if (this->_renderPassResources) {
    descriptorSets[descriptorSetCount] = *this->_renderPassResources;
    ++descriptorSetCount;
  }

  if (this->_subpassResource) {
    descriptorSets[descriptorSetCount] = *this->_subpassResource;
    ++descriptorSetCount;
  }

  if (!material.isEmpty()) {
    descriptorSets[descriptorSetCount] =
        material.getCurrentDescriptorSet(*this->_pFrame);
    ++descriptorSetCount;
  }

  vkCmdBindDescriptorSets(
      this->_commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->_pCurrentSubpass->getPipeline().getLayout(),
      0,
      descriptorSetCount,
      descriptorSets,
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

  // TODO: Only do these checks in debug mode!
  if (!this->_pCurrentSubpass->getPipeline().isDynamicFrontFaceEnabled()) {
    throw std::runtime_error("Attempting to dynamically set front face without "
                             "enabling this feature in the pipeline.");
  }

  vkCmdSetFrontFace(this->_commandBuffer, frontFace);
}

void DrawContext::bindIndexBuffer(const IndexBuffer& indexBuffer) const {
  const BufferAllocation& allocation = indexBuffer.getAllocation();
  vkCmdBindIndexBuffer(
      this->_commandBuffer,
      allocation.getBuffer(),
      allocation.getInfo().offset,
      VK_INDEX_TYPE_UINT32);
}

void DrawContext::drawIndexed(uint32_t indexCount) const {
  // TODO: Expose more flexible draw commands, index offset, vertex offset,
  // instance count, etc.
  vkCmdDrawIndexed(this->_commandBuffer, indexCount, 1, 0, 0, 0);
}

void DrawContext::draw(uint32_t vertexCount) const {
  vkCmdDraw(this->_commandBuffer, vertexCount, 1, 0, 0);
}
} // namespace AltheaEngine