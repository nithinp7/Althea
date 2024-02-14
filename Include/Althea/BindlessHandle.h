#pragma once

#include "Library.h"
#include <cstdint>

namespace AltheaEngine {
#define INVALID_BINDLESS_HANDLE 0xFFFFFFFF
struct ALTHEA_API UniformHandle {
  uint32_t index = INVALID_BINDLESS_HANDLE;

  bool isValid() const { return index != INVALID_BINDLESS_HANDLE; }
};

struct ALTHEA_API BufferHandle {
  uint32_t index = INVALID_BINDLESS_HANDLE;

  bool isValid() const { return index != INVALID_BINDLESS_HANDLE; }
};

struct ALTHEA_API ImageHandle {
  uint32_t index = INVALID_BINDLESS_HANDLE;

  bool isValid() const { return index != INVALID_BINDLESS_HANDLE; }
};

struct ALTHEA_API TlasHandle {
  uint32_t index = INVALID_BINDLESS_HANDLE;

  bool isValid() const { return index != INVALID_BINDLESS_HANDLE; }
};
} // AltheaEngine
