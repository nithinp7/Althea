#include "Serialization.h"
#include <cassert>

namespace AltheaEngine {

thread_local int64_t DeserializationContext::g_ptrOffset = 0;

DeserializationContext::DeserializationContext(
    char* serializedBasePtr,
    char* inPlaceBasePtr) {
  assert(g_ptrOffset == 0);
  g_ptrOffset = inPlaceBasePtr - serializedBasePtr;
}

DeserializationContext::~DeserializationContext() { g_ptrOffset = 0; }
} // namespace AltheaEngine