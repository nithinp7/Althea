#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <string>
#include <vector>


namespace AltheaEngine {
class Utilities {
public:
  static std::vector<char> readFile(const std::string& filename);
};
} // namespace AltheaEngine
