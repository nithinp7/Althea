#include "DefaultTextures.h"
#include "Utilities.h"

#include <CesiumGltf/Sampler.h>
#include <CesiumGltfReader/GltfReader.h>

#include <string>
#include <vector>
#include <gsl/span>

#include <cassert>

using namespace CesiumGltf;
using namespace CesiumGltfReader;

namespace AltheaEngine {
// TODO: Read these paths from somewhere? Config file?
const static std::string NORMAL_1x1_PATH = "../Content/Engine/Textures/normal1x1.png";
const static std::string GREEN_1x1_PATH = "../Content/Engine/Textures/green1x1.png";
const static std::string WHITE_1x1_PATH = "../Content/Engine/Textures/white1x1.png";

std::shared_ptr<Texture> GNormalTexture1x1 = nullptr;
std::shared_ptr<Texture> GGreenTexture1x1 = nullptr; 
std::shared_ptr<Texture> GWhiteTexture1x1 = nullptr; 

namespace {
gsl::span<const std::byte> charVecToByteSpan(const std::vector<char>& buffer) {
  return 
      gsl::span<const std::byte>(
        reinterpret_cast<const std::byte*>(buffer.data()), 
        buffer.size());
}
}

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
    return;
  }

  Sampler sampler;
  sampler.magFilter = Sampler::MagFilter::LINEAR;
  sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  GNormalTexture1x1 = std::make_shared<Texture>(app, *normalResult.image, sampler);
  GGreenTexture1x1 = std::make_shared<Texture>(app, *greenResult.image, sampler);
  GWhiteTexture1x1 = std::make_shared<Texture>(app, *whiteResult.image, sampler);
}

void destroyDefaultTextures() {
  assert(GNormalTexture1x1.use_count() == 1);
  assert(GGreenTexture1x1.use_count() == 1);
  assert(GWhiteTexture1x1.use_count() == 1);

  GNormalTexture1x1 = nullptr;
  GGreenTexture1x1 = nullptr;
  GWhiteTexture1x1 = nullptr;
}
} // namespace AltheaEngine
