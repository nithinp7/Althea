#pragma once

#include "Material.h"
#include "FrameContext.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <memory>

namespace AltheaEngine {

struct DrawContext {
  void bindDescriptorSets(
      const std::unique_ptr<Material>& pMaterial) const;

  VkCommandBuffer commandBuffer;
  std::optional<VkDescriptorSet> globalResources;
  std::optional<VkDescriptorSet> renderPassResources;
  std::optional<VkDescriptorSet> subpassResource;
  VkPipelineLayout pipelineLayout;
  FrameContext frame;
};
} // namespace AltheaEngine