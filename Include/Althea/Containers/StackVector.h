#pragma once

#include "SimpleIterator.h"

#include <cstdint>

namespace AltheaEngine {
template <typename _T> class StackVector {
public:
  typedef SimpleIterator<_T> iterator;
  typedef SimpleIterator<const _T> const_iterator;

  StackVector(_T* pData, size_t capacity)
      : m_pData(pData), m_count(0), m_capacity(capacity) {}

  _T& operator[](size_t idx) { return m_pData[idx]; }
  const _T& operator[](size_t idx) const { return m_pData[idx]; }

  iterator begin() { return {m_pData}; }
  const_iterator begin() const { return {m_pData}; }
  iterator end() { return {&m_pData[m_count]}; }
  const_iterator end() const { return {&m_pData[m_count]}; }

  void push_back(const _T& e) { new (&m_pData[m_count++]) _T(e); }

  void push_back(_T&& e) { new (&m_pData[m_count++]) _T(std::move(e)); }

  template <typename... Args> void emplace_back(Args... args) {
    new (&m_pData[m_count++]) _T(std::forward<Args>(args));
  }

  void clear() {
    for (size_t i = 0; i < m_count; ++i)
      m_pData[i].~_T();
    m_count = 0;
  }

  // TODO: Might want to support removals...

private:
  _T* m_pData;
  size_t m_count;
  size_t m_capacity;
};

#define ALTHEA_STACK_VECTOR(name, _T, capacity)                                \
  StackVector name((_T*)alloca(sizeof(_T) * (capacity)), (capacity))

} // namespace AltheaEngine