#include "Utilities.h"

#include <glm/common.hpp>
#include <glm/glm.hpp>

#include <fstream>

namespace AltheaEngine {
/*static*/
std::vector<char> Utilities::readFile(const std::string& filename) {
  std::ifstream file(filename.c_str(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file!");
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
uint32_t Utilities::computeMipCount(uint32_t width, uint32_t height) {
  return 1 + static_cast<uint32_t>(
                 glm::ceil(glm::log2((double)glm::max(width, height))));
}
} // namespace AltheaEngine
