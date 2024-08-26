#pragma once

#include <cstdint>
#include <vector>

#include <gsl/span>

namespace AltheaEngine {
struct DeserializationContext {
  thread_local static int64_t g_ptrOffset;

  DeserializationContext(char* serializedBasePtr, char* inPlaceBasePtr);

  ~DeserializationContext();
};

#define SCOPED_DESERIALIZE(OLD_BASE_PTR, NEW_BASE_PTR)                         \
  DeserializationContext __deserialization(OLD_BASE_PTR, NEW_BASE_PTR);

#if _DEBUG
template <typename T, typename U = T>
static gsl::span<U> serializeVector(const std::vector<T>& v) {
  gsl::span<U> blockView(
      (U*)LinearAllocator::allocate(sizeof(U) * v.size()),
      v.size());
  for (int i = 0; i < v.size(); ++i)
    new (&blockView[i]) U(v[i]);
  return blockView;
}
#endif

template <typename U, typename T = U>
static std::vector<T> deserializeVector(gsl::span<U> u) {
  u = gsl::span(
      (U*)((char*)&u[0] + DeserializationContext::g_ptrOffset),
      u.size());
  std::vector<T> v;
  v.resize(u.size());
  for (int i = 0; i < u.size(); ++i) {
    v[i] = u[i];
  }
  return v;
}

} // namespace AltheaEngine