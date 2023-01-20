#include "Sampler.h"

#include "Application.h"

#include <stdexcept>

namespace AltheaEngine {
Sampler::Sampler(const Application& app, const SamplerOptions& options)
    : _options(options) {

  bool hasMips = options.mipCount > 1;

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

  samplerInfo.addressModeU = options.addressModeU;
  samplerInfo.addressModeV = options.addressModeV;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  samplerInfo.magFilter = options.magFilter;
  samplerInfo.minFilter = options.minFilter;

  samplerInfo.anisotropyEnable = hasMips;
  samplerInfo.maxAnisotropy =
      hasMips ? app.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy
              : 0.0f;

  samplerInfo.mipmapMode = options.mipmapMode;
  samplerInfo.mipLodBias = options.mipBias;
  samplerInfo.minLod = static_cast<float>(options.firstMip);
  samplerInfo.maxLod = static_cast<float>(options.mipCount - 1);

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  VkDevice device = app.getDevice();
  VkSampler sampler;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create sampler.");
  }

  this->_sampler.set(device, sampler);
}

void Sampler::SamplerDeleter::operator()(VkDevice device, VkSampler sampler) {
  vkDestroySampler(device, sampler, nullptr);
}
} // namespace AltheaEngine