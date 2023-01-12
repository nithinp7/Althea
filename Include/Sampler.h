#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace AltheaEngine {
class Application;

class Sampler {
public:
  Sampler(
      const Application& app, 
      const VkSamplerCreateInfo& createInfo);
  
  Sampler() = default;
  
  // Move-only semantics
  Sampler(Sampler&& rhs);
  Sampler& operator=(Sampler&& rhs);

  Sampler(const Sampler& rhs) = delete;
  Sampler& operator=(const Sampler& rhs) = delete;

  ~Sampler();

  VkSampler getSampler() const {
    return this->_sampler;
  }
  
private:
  VkDevice _device = VK_NULL_HANDLE;
  VkSampler _sampler = VK_NULL_HANDLE;
};
} // namespace AltheaEngine