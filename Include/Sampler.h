#pragma once

#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

class Sampler {
public:
  Sampler(const Application& app, const VkSamplerCreateInfo& createInfo);
  Sampler() = default;

  operator VkSampler() const { return this->_sampler; }

private:
  struct SamplerDeleter {
    void operator()(VkDevice device, VkSampler sampler);
  };

  UniqueVkHandle<VkSampler, SamplerDeleter> _sampler;
};
} // namespace AltheaEngine