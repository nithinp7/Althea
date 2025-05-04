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
#include <cstdint>

namespace AltheaEngine {
/*static*/
std::vector<char> Utilities::readFile(const char* filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    char buf[2048];
    sprintf(buf, "Failed to open file: %s", filename);
    throw std::runtime_error(buf);
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
std::vector<char> Utilities::readFile(const std::string& filename) {
  return readFile(filename.c_str());
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
bool Utilities::checkFileExists(const char* filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  return file.good();
}

/*static*/
bool Utilities::checkFileExists(const std::string& filename) {
  return checkFileExists(filename.c_str());
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
void Utilities::loadImage(const char* path, Utilities::ImageFile& result) {
  result.channels = 4;
  result.bytesPerChannel = 1;

  int originalChannels;
  stbi_uc* pImg = stbi_load(
      path,
      &result.width,
      &result.height,
      &originalChannels,
      result.channels);

  result.data.resize(
      result.width * result.height * result.channels * result.bytesPerChannel);
  std::memcpy(result.data.data(), pImg, result.data.size());
  stbi_image_free(pImg);
}

/*static*/
void Utilities::loadImage(
    const std::string& path,
    Utilities::ImageFile& result) {
  loadImage(path.c_str(), result);
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
void Utilities::loadHdri(
    const std::string& path,
    Utilities::ImageFile& result) {
  std::vector<char> data = Utilities::readFile(path);

  result.channels = 4;
  result.bytesPerChannel = 4;

  int32_t originalChannels;
  float* pHdriImage = stbi_loadf_from_memory(
      reinterpret_cast<stbi_uc*>(data.data()),
      static_cast<int>(data.size()),
      &result.width,
      &result.height,
      &originalChannels,
      4);

  result.data.resize(
      result.width * result.height * result.channels * result.bytesPerChannel);
  std::memcpy(result.data.data(), pHdriImage, result.data.size());

  stbi_image_free(pHdriImage);
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

namespace TiffLoaderImpl {
struct TiffParser {
  TiffParser(const char* filepath)
      : m_buffer(Utilities::readFile(filepath)), m_offset(0) {}

  template <typename T> const T& parse() {
    const T& t = *reinterpret_cast<const T*>(&m_buffer[m_offset]);
    m_offset += sizeof(T);
    return t;
  }

  template <typename T> const T& peak() {
    return *reinterpret_cast<const T*>(&m_buffer[m_offset]);
  }

  void seek(uint32_t offset) { m_offset = offset; }

  struct ScopedSeek {
    ScopedSeek(TiffParser* p, uint32_t offset)
        : m_parser(p), m_prevOffset(p->m_offset) {
      m_parser->m_offset = offset;
    }

    ~ScopedSeek() { m_parser->m_offset = m_prevOffset; }

    TiffParser* m_parser;
    uint32_t m_prevOffset;
  };

  ScopedSeek pushSeek(uint32_t offset) { return ScopedSeek(this, offset); }

  std::vector<char> m_buffer;
  uint32_t m_offset;
};
} // namespace TiffLoaderImpl

/*static*/
void Utilities::loadTiff(const char* filepath, ImageFile& res) {
  using namespace TiffLoaderImpl;

  TiffParser p(filepath);

  uint32_t pStripOffsets = 0;
  uint32_t stripOffsetStride = 0;
  uint32_t pStripByteSizes = 0;
  uint32_t stripByteSizeStride = 0;
  uint32_t rowsPerStrip = 0;
  uint32_t stripCount = 0;
  bool bPackBits = false;

  // IFH
  struct IFH {
    uint16_t byteOrder;
    uint16_t magic;
    uint32_t ifdOffset;
  };
  const IFH& header = p.parse<IFH>();
  assert(header.byteOrder == 0x4949); // little-endian
  assert(header.magic == 42);

  enum TagType : uint16_t {
    ImageWidth = 0x100,
    ImageLength = 0x101,
    BitsPerSample = 0x102,
    Compression = 0x103,
    PhotometricInterpretation = 0x106,
    StripOffsets = 0x111,
    SamplesPerPixel = 0x115, // ??
    RowsPerStrip = 0x116,
    StripByteCounts = 0x117,
    XResolution = 0x11a, // ??
    YResolution = 0x11b, // ??
    PlanarConfiguration = 0x11c,
    ResolutionUnit = 0x128,
    SampleFormat = 0x153
    // TODO: expand support as needed...
  };

  enum FieldType : uint16_t {
    BYTE = 1,
    ASCII = 2,
    SHORT = 3,
    LONG = 4,
    RATIONAL = 5,
    SBYTE = 6,
    UNDEFINED = 7,
    SSHORT = 8,
    SLONG = 9,
    SRATIONAL = 10,
    FLOAT = 11,
    DOUBLE = 12
  };

  // IFD
  struct IFDEntry {
    TagType tag;
    FieldType fieldType;
    uint32_t count;
    uint32_t valueOffset;
  };

  uint32_t nextIfdOffset = header.ifdOffset;
  while (nextIfdOffset != 0) {
    p.seek(nextIfdOffset);
    uint16_t numEntries = p.parse<uint16_t>();

    // TODO:
    for (int i = 0; i < numEntries; i++) {
      const IFDEntry& entry = p.parse<IFDEntry>();

      //printf("IFD ENTRY - tag: %u, fieldType: %u, count: %u, valueOffset: %u\n", entry.tag, entry.fieldType, entry.count, entry.valueOffset);

      switch (entry.tag)
      {
      case ImageWidth: {
        res.width = entry.valueOffset;
        break;
      };
      case ImageLength: {
        res.height = entry.valueOffset;
        break;
      };
      case BitsPerSample: {
        assert((entry.valueOffset & 7) == 0);
        res.bytesPerChannel = entry.valueOffset >> 3;
        break;
      };
      case Compression: {
        // TODO support other types of compression as needed
        assert(entry.valueOffset == 32773); // PackBits
        // ...
        bPackBits = true;
        break;
      };
      case StripOffsets: {
        pStripOffsets = entry.valueOffset;
        assert(stripCount == 0 || stripCount == entry.count);
        stripCount = entry.count;
        stripOffsetStride = (entry.fieldType == SHORT) ? 2 : 4;
        break;
      };
      case SamplesPerPixel: {
        res.channels = entry.valueOffset;
        break;
      };
      case RowsPerStrip: {
        rowsPerStrip = entry.valueOffset;
        break;
      };
      case StripByteCounts: {
        pStripByteSizes = entry.valueOffset;
        assert(stripCount == 0 || stripCount == entry.count);
        stripCount = entry.count;
        stripByteSizeStride = (entry.fieldType == SHORT) ? 2 : 4;
        break;
      };
      default:
        continue;
      }
    }

    nextIfdOffset = p.parse<uint32_t>();
  }

  uint32_t outputImgSize = res.width * res.height * res.channels * res.bytesPerChannel;
  res.data.resize(outputImgSize);
  uint32_t dstOffset = 0;
  assert(stripCount != 0);
  for (uint32_t stripIdx = 0; stripIdx < stripCount; stripIdx++)
  {
    p.seek(pStripByteSizes + stripIdx * stripByteSizeStride);
    uint32_t byteCount = (stripByteSizeStride == 2) ? p.parse<uint16_t>() : p.parse<uint32_t>();
    p.seek(pStripOffsets + stripIdx * stripOffsetStride);
    uint32_t stripOffset = (stripOffsetStride == 2) ? p.parse<uint16_t>() : p.parse<uint32_t>();

    if (bPackBits)
    {
      for (uint32_t byteOffset = 0; byteOffset < byteCount;) {
        assert(dstOffset < outputImgSize);
        p.seek(stripOffset + byteOffset);
        char n = p.parse<char>();
        byteOffset++;
        if (n == -128) {
          // just padding, skip
        }
        else if (n < 0) {
          // next byte copied 1-n times
          uint32_t repeatedRunCount = 1 - (int)n;
          memset(&res.data[dstOffset], p.parse<uint8_t>(), repeatedRunCount);
          dstOffset += repeatedRunCount;
          byteOffset++;
        }
        else {
          // literal run of n+1 bytes
          uint32_t literalRunCount = n + 1;
          memcpy(&res.data[dstOffset], &p.peak<uint8_t>(), literalRunCount);
          dstOffset += literalRunCount;
          byteOffset += literalRunCount;
        }
      }
    }
    else {
      memcpy(&res.data[dstOffset], &p.peak<uint8_t>(), byteCount);
      dstOffset += byteCount;
    }
  }

  assert(dstOffset == outputImgSize);
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
