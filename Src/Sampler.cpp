#include "Sampler.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Sampler::Sampler(
    const Application& app,
    const VkSamplerCreateInfo& createInfo) {
  VkDevice device = app.getDevice();
  VkSampler sampler;
  if (vkCreateSampler(device, &createInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create sampler.");
  }

  this->_sampler.set(device, sampler);
}

void Sampler::SamplerDeleter::operator()(VkDevice device, VkSampler sampler) {
  vkDestroySampler(device, sampler, nullptr);
}
} // namespace AltheaEngine