#include "Utilities.h"

#include <CesiumGltf/ImageCesium.h>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <tinyexr.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb_image_write.h"

#define CHANNEL_NUM 3

#include <cstdlib>
#include <fstream>

namespace AltheaEngine {
/*static*/
std::vector<char> Utilities::readFile(const std::string& filename) {
  std::ifstream file(filename.c_str(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
    return std::vector<char>();
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

/*static*/
bool Utilities::writeFile(
    const std::string& filename,
    gsl::span<const char> data) {
  std::ofstream file(filename.c_str(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
    return false;
  }

  file.seekp(0);
  file.write(data.data(), data.size());

  file.close();

  return true;
}

/*static*/
bool Utilities::checkFileExists(const std::string& filename) {
  std::ifstream file(filename.c_str(), std::ios::ate | std::ios::binary);
  return file.good();
}

/*static*/
uint32_t Utilities::computeMipCount(uint32_t width, uint32_t height) {
  return 1 + static_cast<uint32_t>(
                 glm::floor(glm::log2((double)glm::max(width, height))));
}

/*static*/
void Utilities::loadPng(const std::string& path, Utilities::ImageFile& result) {
  std::vector<char> data = Utilities::readFile(path);

  result.channels = 4;
  result.bytesPerChannel = 1;

  int originalChannels;
  stbi_uc* pPngImage = stbi_load_from_memory(
    reinterpret_cast<const stbi_uc*>(data.data()),
    static_cast<int>(data.size()),
    &result.width,
    &result.height,
    &originalChannels,
    4);

  result.data.resize(
    result.width * result.height * result.channels * result.bytesPerChannel);
  std::memcpy(result.data.data(), pPngImage, result.data.size());
  stbi_image_free(pPngImage);
}

/*static*/
CesiumGltf::ImageCesium Utilities::loadPng(const std::string& path) {
  std::vector<char> data = Utilities::readFile(path);

  CesiumGltf::ImageCesium image;
  image.channels = 4;
  image.bytesPerChannel = 1;

  int32_t originalChannels;
  stbi_uc* pPngImage = stbi_load_from_memory(
      reinterpret_cast<const stbi_uc*>(data.data()),
      static_cast<int>(data.size()),
      &image.width,
      &image.height,
      &originalChannels,
      4);

  image.pixelData.resize(
      image.width * image.height * image.channels * image.bytesPerChannel);
  std::memcpy(image.pixelData.data(), pPngImage, image.pixelData.size());
  stbi_image_free(pPngImage);

  return std::move(image);
}

/*static*/
void Utilities::savePng(
    const std::string& path,
    int width,
    int height,
    gsl::span<const std::byte> data) {
  stbi_write_png(path.c_str(), width, height, 4, data.data(), 0);
}

/*static*/
CesiumGltf::ImageCesium Utilities::loadHdri(const std::string& path) {
  std::vector<char> data = Utilities::readFile(path);

  CesiumGltf::ImageCesium image;
  image.channels = 4;
  image.bytesPerChannel = 4;

  int32_t originalChannels;
  float* pHdriImage = stbi_loadf_from_memory(
      reinterpret_cast<stbi_uc*>(data.data()),
      static_cast<int>(data.size()),
      &image.width,
      &image.height,
      &originalChannels,
      4);

  image.pixelData.resize(
      image.width * image.height * image.channels * image.bytesPerChannel);
  std::memcpy(image.pixelData.data(), pHdriImage, image.pixelData.size());

  stbi_image_free(pHdriImage);

  return std::move(image);
}

/*static*/
void Utilities::saveHdri(
    const std::string& path,
    int width,
    int height,
    gsl::span<const std::byte> data) {
  stbi_write_hdr(
      path.c_str(),
      width,
      height,
      4,
      reinterpret_cast<const float*>(data.data()));
}

/*static*/
void Utilities::saveExr(
    const std::string& path,
    int width,
    int height,
    gsl::span<const std::byte> data) {
  SaveEXR(
      reinterpret_cast<const float*>(data.data()),
      width,
      height,
      4,
      0,
      path.c_str(),
      nullptr);
}

std::vector<uint32_t> Utilities::randSeedStack;
uint32_t Utilities::randSeed = 0;

/*static*/
void Utilities::pushRandSeed(uint32_t seed) {
  randSeedStack.push_back(randSeed);
  randSeed = seed;
}

/*static*/
uint32_t Utilities::getRandSeed() { return randSeed; }

/*static*/
float Utilities::randf(uint32_t s) { return (float)rand(s) / RAND_MAX; }

/*static*/
float Utilities::randf() { return (float)rand() / RAND_MAX; }

/*static*/
int Utilities::rand(uint32_t s) {
  std::srand(s);
  return std::rand();
}

/*static*/
int Utilities::rand() {
  std::srand(randSeed);
  randSeed = randSeed * 214013 + 2531011;
  return std::rand();
}

/*static*/
void Utilities::popRandSeed() {
  randSeed = randSeedStack.back();
  randSeedStack.pop_back();
}

} // namespace AltheaEngine
