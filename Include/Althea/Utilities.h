#pragma once

#include "Library.h"

#include <gsl/span>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumGltf {
struct ImageCesium;
};

namespace AltheaEngine {
class ALTHEA_API Utilities {
public:
  static std::vector<char> readFile(const std::string& filename);
  static uint32_t computeMipCount(uint32_t width, uint32_t height);

  static CesiumGltf::ImageCesium loadPng(const std::string& path);
  static void savePng(
      const std::string& path,
      int width,
      int height,
      gsl::span<const std::byte> data);

  static CesiumGltf::ImageCesium loadHdri(const std::string& path);
  static void saveHdri(
      const std::string& path,
      int width,
      int height,
      gsl::span<const std::byte> data);
};
} // namespace AltheaEngine
