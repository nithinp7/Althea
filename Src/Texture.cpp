#include "Texture.h"

#include <CesiumGltf/Model.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/ImageCesium.h>

#include <memory>

Texture::Texture(
    const Application& app,
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture) 
  : _device(app.getDevice()) {
  if (texture.sampler < 0 || texture.sampler >= model.samplers.size() ||
      texture.source < 0 || texture.source >= model.images.size()) {
    return;
  }

  const CesiumGltf::Sampler& sampler = model.samplers[texture.sampler];
  const CesiumGltf::ImageCesium& image = model.images[texture.source].cesium;

  // TODO: support compressed pixel formats
  if (image.compressedPixelFormat != CesiumGltf::GpuCompressedPixelFormat::NONE) {
    return;
  }
  
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  
  if (sampler.magFilter) {
    switch (*sampler.magFilter) {
      case CesiumGltf::Sampler::MagFilter::NEAREST:
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        break;
      default:
        samplerInfo.magFilter = VK_FILTER_LINEAR;
    }
  } else {
    samplerInfo.magFilter = VK_FILTER_LINEAR;
  }

  if (sampler.minFilter) {
    switch (*sampler.minFilter) {
      case CesiumGltf::Sampler::MinFilter::NEAREST:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        break;
      //case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
      //case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
      //case CesiumGltf::Sampler::MinFilter::LINEAR:
      default:
        samplerInfo.minFilter = VK_FILTER_LINEAR;
    }

    switch(*sampler.minFilter) {
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;
      //case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
      //case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
      // ??
      //case CesiumGltf::Sampler::MinFilter::NEAREST:
      //case CesiumGltf::Sampler::MinFilter::LINEAR:
      default:
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
  } else {
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }

  // TODO: revisit mipmapping
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 
      app.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
  
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  if (vkCreateSampler(this->_device, &samplerInfo, nullptr, &this->_textureSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create texture sampler!");
  }

  app.createTextureImage(
      (void*)image.pixelData.data(),
      image.pixelData.size(),
      image.width,
      image.height,
      VK_FORMAT_R8G8B8A8_SRGB,
      this->_textureImage,
      this->_textureImageMemory);

  this->_textureImageView = 
      app.createImageView(this->_textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

Texture::Texture(Texture&& rhs) 
  : _device(rhs._device),
    _textureImage(rhs._textureImage),
    _textureImageMemory(rhs._textureImageMemory),
    _textureImageView(rhs._textureImageView),
    _textureSampler(rhs._textureSampler) {
  rhs._needsDestruction = false;
}

Texture::~Texture() {
  if (!this->_needsDestruction) {
    return;
  }

  vkDestroySampler(this->_device, this->_textureSampler, nullptr);
  vkDestroyImageView(this->_device, this->_textureImageView, nullptr);

  vkDestroyImage(this->_device, this->_textureImage, nullptr);
  vkFreeMemory(this->_device, this->_textureImageMemory, nullptr);
}