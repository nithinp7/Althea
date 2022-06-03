#pragma once

#include "Application.h"

#include <vulkan/vulkan.h>

namespace CesiumGltf {
struct Model;
struct Texture;
} // namespace CesiumGltf

class Texture {
public: 
  Texture(
      const Application& app,
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture);
  Texture(Texture&& rhs);
  ~Texture();
private:
  VkDevice _device;

  VkImage _textureImage;
  VkDeviceMemory _textureImageMemory;

  bool _needsDestruction = true;
};

