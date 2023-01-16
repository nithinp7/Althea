#include "PipelineLayout.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {

PipelineLayoutBuilder& PipelineLayoutBuilder::addDescriptorSet(
    VkDescriptorSetLayout descriptorSetLayout) {
  this->_descriptorSetLayouts.push_back(descriptorSetLayout);
  return *this;
}

PipelineLayout::PipelineLayout(
    const Application& app,
    const PipelineLayoutBuilder& builder)
    : _pushConstantRanges(builder._pushConstantRanges) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      static_cast<uint32_t>(builder._descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = builder._descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount =
      static_cast<uint32_t>(builder._pushConstantRanges.size());
  pipelineLayoutInfo.pPushConstantRanges = builder._pushConstantRanges.data();

  VkDevice device = app.getDevice();
  VkPipelineLayout pipelineLayout;
  if (vkCreatePipelineLayout(
          device,
          &pipelineLayoutInfo,
          nullptr,
          &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }

  this->_layout.set(device, pipelineLayout);
}

void PipelineLayout::PipelineLayoutDeleter::operator()(
    VkDevice device,
    VkPipelineLayout pipelineLayout) {
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}
} // namespace AltheaEngine