#include "Sampler.h"

#include "Application.h"
#include "Utilities.h"

#include <CesiumGltf/Sampler.h>

#include <stdexcept>

namespace AltheaEngine {
SamplerOptions::SamplerOptions(const CesiumGltf::Sampler& sampler, uint32_t mipCount)
{
  switch (sampler.wrapS) {
  case CesiumGltf::Sampler::WrapS::MIRRORED_REPEAT:
    this->addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    break;
  case CesiumGltf::Sampler::WrapS::REPEAT:
    this->addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    break;
  // case CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE:
  default:
    this->addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  };

  switch (sampler.wrapT) {
  case CesiumGltf::Sampler::WrapT::MIRRORED_REPEAT:
    this->addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    break;
  case CesiumGltf::Sampler::WrapT::REPEAT:
    this->addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    break;
  // case CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE:
  default:
    this->addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  };

  if (sampler.magFilter) {
    switch (*sampler.magFilter) {
    case CesiumGltf::Sampler::MagFilter::NEAREST:
      this->magFilter = VK_FILTER_NEAREST;
      break;
    default:
      this->magFilter = VK_FILTER_LINEAR;
    }
  } else {
    this->magFilter = VK_FILTER_LINEAR;
  }

  bool useMipMaps = false;

  // Determine minification filter
  int32_t minFilter = sampler.minFilter.value_or(
      CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR);

  switch (minFilter) {
  case CesiumGltf::Sampler::MinFilter::NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
    this->minFilter = VK_FILTER_NEAREST;
    break;
  // case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
  // case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  // case CesiumGltf::Sampler::MinFilter::LINEAR:
  default:
    this->minFilter = VK_FILTER_LINEAR;
  }

  // Determine mipmap mode.
  switch (minFilter) {
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
    useMipMaps = true;
    this->mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    break;
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
    useMipMaps = true;
    this->mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    break;
  case CesiumGltf::Sampler::MinFilter::NEAREST:
  case CesiumGltf::Sampler::MinFilter::LINEAR:
  default:
    useMipMaps = false;
    this->mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }

  this->mipCount = useMipMaps ? mipCount : 1;
}

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
  samplerInfo.unnormalizedCoordinates = !options.normalized;

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