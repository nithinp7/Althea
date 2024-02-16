#pragma once

#include "Library.h"
#include <cstdint>

namespace AltheaEngine {
#define INVALID_BINDLESS_HANDLE 0xFFFFFFFF

#define DECL_BINDLESS_HANDLE_TYPE(NAME)                               \
  struct ALTHEA_API NAME {                                            \
    uint32_t index = INVALID_BINDLESS_HANDLE;                         \
    bool isValid() const { return index != INVALID_BINDLESS_HANDLE; } \
  };

DECL_BINDLESS_HANDLE_TYPE(UniformHandle)
DECL_BINDLESS_HANDLE_TYPE(BufferHandle)
DECL_BINDLESS_HANDLE_TYPE(TextureHandle)
DECL_BINDLESS_HANDLE_TYPE(ImageHandle)
DECL_BINDLESS_HANDLE_TYPE(TlasHandle)
} // AltheaEngine
