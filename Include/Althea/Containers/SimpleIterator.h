#pragma once

#include <cstdint>
#include <memory>

namespace AltheaEngine {

// Wrapper to avoid using pointers as iterators directly.
// In particular this avoids confusion arising from container.end()
// returning a non-null pointer.

template <typename _T> struct SimpleIterator {
  _T* ptr;

  _T& operator*() { return *ptr; }
  void operator++() { ptr++; }
  bool operator!=(SimpleIterator<_T> other) const { return ptr != other.ptr; }
};

template <typename _T> struct StridedIterator {
  std::byte* ptr;
  size_t stride;

  _T& operator*() { return *reinterpret_cast<_T*>(ptr); }
  void operator++() { ptr += stride; }
  bool operator!=(StridedIterator<_T> other) const { return ptr != other.ptr; }
};

template <typename _T> struct ConstStridedIterator {
  const std::byte* ptr;
  size_t stride;

  const _T& operator*() { return *reinterpret_cast<const _T*>(ptr); }
  void operator++() { ptr += stride; }
  bool operator!=(ConstStridedIterator<_T> other) const { return ptr != other.ptr; }
};
} // namespace AltheaEngine