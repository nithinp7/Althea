#include "Sampler.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Sampler::Sampler(
    const Application& app,
    const VkSamplerCreateInfo& createInfo) {
  VkSampler sampler;
  if (vkCreateSampler(app.getDevice(), &createInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create sampler.");
  }

  this->_sampler =
      UniqueVkHandle<VkSampler, SamplerDeleter>(app.getDevice(), sampler);
}

void Sampler::SamplerDeleter::operator()(VkDevice device, VkSampler sampler) {
  vkDestroySampler(device, sampler, nullptr);
}
} // namespace AltheaEngine