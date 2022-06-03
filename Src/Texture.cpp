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
  
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  app.createBuffer(
      image.pixelData.size(), 
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer,
      stagingBufferMemory);

  void* data;
  vkMapMemory(this->_device, stagingBufferMemory, 0, image.pixelData.size(), 0, &data);
  std::memcpy(data, image.pixelData.data(), image.pixelData.size());
  vkUnmapMemory(this->_device, stagingBufferMemory);

  app.createImage(
    static_cast<uint32_t>(image.width),
    static_cast<uint32_t>(image.height),
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    this->_textureImage,
    this->_textureImageMemory);

  vkDestroyBuffer(this->_device, stagingBuffer, nullptr);
  vkFreeMemory(this->_device, stagingBufferMemory, nullptr);
}