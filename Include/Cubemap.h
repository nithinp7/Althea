#pragma once

#include <CesiumGltf/ImageCesium.h>
#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace AltheaEngine {
class Application;

class Cubemap {
public:
  Cubemap(Application& app, const std::array<std::string, 6>& cubemapPaths);
  Cubemap(Application& app, const std::array<CesiumGltf::ImageCesium, 6>& images);
  ~Cubemap();

  VkImage getImage() const {
    return this->_image;
  }

  VkImageView getImageView() const {
    return this->_imageView;
  }

  VkSampler getSampler() const {
    return this->_imageSampler;
  }

private:
  void _initCubemap(
      Application& app, 
      const std::array<CesiumGltf::ImageCesium, 6>& images);

  VkDevice _device;

  VkImage _image;
  VkDeviceMemory _imageMemory;

  VkImageView _imageView;
  VkSampler _imageSampler;
};
} // AltheaEngine