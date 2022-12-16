#include "Utilities.h"

#include <fstream>

namespace AltheaEngine {
/*static*/
std::vector<char> Utilities::readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

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
} // namespace AltheaEngine
