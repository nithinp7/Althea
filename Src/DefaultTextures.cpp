#include "DefaultTextures.h"
#include "Utilities.h"

#include <CesiumGltf/Sampler.h>
#include <CesiumGltfReader/GltfReader.h>

#include <string>
#include <vector>
#include <gsl/span>
#include <memory>

using namespace CesiumGltf;
using namespace CesiumGltfReader;

namespace AltheaEngine {
const static std::string NORMAL_1x1_PATH = "./Content/Engine/normal1x1.png";
const static std::string GREEN_1x1_PATH = "./Content/Engine/green1x1.png";
const static std::string WHITE_1x1_PATH = "./Content/Engine/white1x1.png";

// /*static*/
// std::unique_ptr<Texture> GNormalTexture1x1 = nullptr;

// /*static*/
// std::unique_ptr<Texture> GGreenTexture1x1 = nullptr;

// /*static*/
// std::unique_ptr<Texture> GWhiteTexture1x1 = nullptr;

namespace {
gsl::span<const std::byte> charVecToByteSpan(const std::vector<char>& buffer) {
  return 
      gsl::span<const std::byte>(
        reinterpret_cast<const std::byte*>(buffer.data()), 
        buffer.size());
}
}

/*static*/ 
void initDefaultTextures(const Application& app) {
  std::vector<char> rawNormal1x1 = Utilities::readFile(NORMAL_1x1_PATH);
  std::vector<char> rawGreen1x1 = Utilities::readFile(GREEN_1x1_PATH);
  std::vector<char> rawWhite1x1 = Utilities::readFile(WHITE_1x1_PATH);

  // TODO: Use KTX2
  Ktx2TranscodeTargets noTranscodeTargets;
  ImageReaderResult normalResult = 
      GltfReader::readImage(charVecToByteSpan(rawNormal1x1), noTranscodeTargets);
  ImageReaderResult greenResult = 
      GltfReader::readImage(charVecToByteSpan(rawGreen1x1), noTranscodeTargets);
  ImageReaderResult whiteResult = 
      GltfReader::readImage(charVecToByteSpan(rawWhite1x1), noTranscodeTargets);

  if (!normalResult.image || !greenResult.image || !whiteResult.image) {
    throw std::runtime_error("Failed to initialize default textures!");
  }

  Sampler sampler;
  sampler.magFilter = Sampler::MagFilter::LINEAR;
  sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  GNormalTexture1x1 = std::make_unique<Texture>(app, *normalResult.image, sampler);
  GGreenTexture1x1 = std::make_unique<Texture>(app, *greenResult.image, sampler);
  GWhiteTexture1x1 = std::make_unique<Texture>(app, *whiteResult.image, sampler);
}

/*static*/ 
void destroyDefaultTextures() {
  GNormalTexture1x1.reset(nullptr);
  GGreenTexture1x1.reset(nullptr);
  GWhiteTexture1x1.reset(nullptr);
}
} // namespace AltheaEngine
