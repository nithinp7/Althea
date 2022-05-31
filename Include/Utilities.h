#pragma once

#include <vector>
#include <string>
#include <optional>

#include <vulkan/vulkan.h>

class Utilities
{
public:
  static std::vector<char> readFile(const std::string& filename);
  static uint32_t findMemoryType(
      const VkPhysicalDevice& physicalDevice,
      uint32_t typeFilter,
      const VkMemoryPropertyFlags& properties);
};

