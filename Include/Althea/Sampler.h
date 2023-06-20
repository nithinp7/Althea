#pragma once

#include "Library.h"
#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

struct ALTHEA_API SamplerOptions {
  VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  VkFilter magFilter = VK_FILTER_LINEAR;
  VkFilter minFilter = VK_FILTER_LINEAR;

  VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  float mipBias = 0.0f;
  uint32_t firstMip = 0;
  uint32_t mipCount = 1;
  bool normalized = true;
};

class ALTHEA_API Sampler {
public:
  Sampler(const Application& app, const SamplerOptions& options);
  Sampler() = default;

  operator VkSampler() const { return this->_sampler; }

  const SamplerOptions& getOptions() const { return this->_options; }

private:
  struct SamplerDeleter {
    void operator()(VkDevice device, VkSampler sampler);
  };

  UniqueVkHandle<VkSampler, SamplerDeleter> _sampler;
  SamplerOptions _options;
};
} // namespace AltheaEngine