#pragma once

#include "Library.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace AltheaEngine {
class ALTHEA_API Utilities {
public:
  static std::vector<char> readFile(const std::string& filename);
  static uint32_t computeMipCount(uint32_t width, uint32_t height);
};
} // namespace AltheaEngine
