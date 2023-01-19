#pragma once

#include "Library.h"

#include "UniqueVkHandle.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

class ALTHEA_API Sampler {
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