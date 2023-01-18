#pragma once

#include "Library.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <string>
#include <vector>


namespace AltheaEngine {
class ALTHEA_API Utilities {
public:
  static std::vector<char> readFile(const std::string& filename);
};
} // namespace AltheaEngine
