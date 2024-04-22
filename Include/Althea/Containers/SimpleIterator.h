#pragma once

namespace AltheaEngine {

// Wrapper to avoid using pointers as iterators directly.
// In particular this avoids confusion arising from container.end()
// returning a non-null pointer.

template <typename _T> struct SimpleIterator {
  _T* ptr;

  _T& operator*() { return *ptr; }
  void operator++() { ptr++; }
  bool operator!=(SimpleIterator other) const { return ptr != other.ptr; }
};
} // namespace AltheaEngine