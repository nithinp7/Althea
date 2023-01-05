#include "DrawContext.h"

namespace AltheaEngine {
void DrawContext::bindDescriptorSets(
    const std::unique_ptr<Material>& pMaterial) const {
  VkDescriptorSet descriptorSets[4];

  uint32_t descriptorSetCount = 0;
  if (this->globalResources) {
    descriptorSets[descriptorSetCount] = *this->globalResources;
    ++descriptorSetCount;
  }

  if (this->renderPassResources) {
    descriptorSets[descriptorSetCount] = *this->renderPassResources;
    ++descriptorSetCount;
  }

  if (this->subpassResource) {
    descriptorSets[descriptorSetCount] = *this->subpassResource;
    ++descriptorSetCount;
  }

  if (pMaterial) {
    descriptorSets[descriptorSetCount] = 
        pMaterial->getCurrentDescriptorSet(this->frame);
    ++descriptorSetCount;
  }

  vkCmdBindDescriptorSets(
      this->commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->pipelineLayout,
      0,
      descriptorSetCount,
      descriptorSets,
      0,
      nullptr);
}
} // namespace AltheaEngine