#pragma once

#include "Library.h"

#include "DescriptorSet.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace AltheaEngine {
class Application;
class PipelineLayout;

class ALTHEA_API PipelineLayoutBuilder {
public:
  /**
   * @brief Add push constants for this pipeline. Later, push constant ranges
   * can be updated in the command buffer before dispatching compute commands.
   *
   * @param stageFlags The shader stages that this push constant range should be
   * made available in.
   * @return This builder.
   */
  template <typename TPushConstants>
  PipelineLayoutBuilder&
  addPushConstants(VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL) {

    uint32_t offset = 0;
    if (!this->_pushConstantRanges.empty()) {
      const VkPushConstantRange& prevRange = this->_pushConstantRanges.back();
      offset = prevRange.offset + prevRange.size;
    }

    VkPushConstantRange& range = this->_pushConstantRanges.emplace_back();
    range.offset = offset;
    range.size = static_cast<uint32_t>(sizeof(TPushConstants));
    range.stageFlags = stageFlags;

    return *this;
  }

  PipelineLayoutBuilder&
  addDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);

private:
  friend class PipelineLayout;

  std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
  std::vector<VkPushConstantRange> _pushConstantRanges;
};

class ALTHEA_API PipelineLayout {
public:
  PipelineLayout() = default;
  PipelineLayout(const Application& app, const PipelineLayoutBuilder& builder);

  operator VkPipelineLayout() const { return this->_layout; }

  const std::vector<VkPushConstantRange>& getPushConstantRanges() const {
    return this->_pushConstantRanges;
  }

private:
  struct PipelineLayoutDeleter {
    void operator()(VkDevice device, VkPipelineLayout pipelineLayout);
  };

  UniqueVkHandle<VkPipelineLayout, PipelineLayoutDeleter> _layout;
  std::vector<VkPushConstantRange> _pushConstantRanges;
};
} // namespace AltheaEngine