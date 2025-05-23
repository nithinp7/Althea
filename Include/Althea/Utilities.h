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
  static std::vector<char> readFile(const char* filename);
  static std::vector<char> readFile(const std::string& filename);
  static bool writeFile(const std::string& fileName, gsl::span<const char> data);
  static bool checkFileExists(const char* filename);
  static bool checkFileExists(const std::string& filename);
  
  static uint32_t computeMipCount(uint32_t width, uint32_t height);

  struct ImageFile {
    int width;
    int height;
    int channels;
    int bytesPerChannel;
    std::vector<std::byte> data;
  };
  static void loadPng(const std::string& path, ImageFile& result);
  static CesiumGltf::ImageCesium loadPng(const std::string& path);
  static void savePng(
      const std::string& path,
      int width,
      int height,
      gsl::span<const std::byte> data);

  static void loadImage(const char* path, ImageFile& result);
  static void loadImage(const std::string& path, ImageFile& result);

  static void loadHdri(const std::string& path, ImageFile& result);
  static CesiumGltf::ImageCesium loadHdri(const std::string& path);
  static void saveHdri(
      const std::string& path,
      int width,
      int height,
      gsl::span<const std::byte> data);
  static void saveExr(
      const std::string& path,
      int width,
      int height,
      gsl::span<const std::byte> data);

  static void loadTiff(const char* path, ImageFile& result);

  static void pushRandSeed(uint32_t seed);
  static uint32_t getRandSeed();
  static float randf(uint32_t s);
  static float randf();
  static int rand(uint32_t s);
  static int rand();
  static void popRandSeed();
private:
  static std::vector<uint32_t> randSeedStack;
  static uint32_t randSeed;
};
} // namespace AltheaEngine
