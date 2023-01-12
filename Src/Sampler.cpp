#include "Sampler.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Sampler::Sampler(
    const Application& app,
    const VkSamplerCreateInfo& createInfo) 
    : _device(app.getDevice()) {
  if (vkCreateSampler(this->_device, &createInfo, nullptr, &this->_sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create sampler.");
  }
}

Sampler::Sampler(Sampler&& rhs)
    : _device(rhs._device),
      _sampler(rhs._sampler) {
  rhs._device = VK_NULL_HANDLE;
  rhs._sampler = VK_NULL_HANDLE;
}

Sampler& Sampler::operator=(Sampler&& rhs) {
  this->_device = rhs._device;
  this->_sampler = rhs._sampler;

  rhs._device = VK_NULL_HANDLE;
  rhs._sampler = VK_NULL_HANDLE;

  return *this;
}

Sampler::~Sampler() {
  if (this->_sampler != VK_NULL_HANDLE) {
    vkDestroySampler(this->_device, this->_sampler, nullptr);
  }
}
} // namespace AltheaEngine