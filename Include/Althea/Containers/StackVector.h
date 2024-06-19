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

  _T* begin_ptr() { return {m_pData}; }
  const _T* begin_ptr() const { return {m_pData}; }
  _T* end_ptr() { return {&m_pData[m_count]}; }
  const _T* end_ptr() const { return {&m_pData[m_count]}; }

  void push_back(const _T& e) {
    assert(m_count < m_capacity);
    new (&m_pData[m_count++]) _T(e);
  }

  void push_back(_T&& e) {
    assert(m_count < m_capacity);
    new (&m_pData[m_count++]) _T(std::move(e));
  }

  template <typename... Args> _T& emplace_back() {
    assert(m_count < m_capacity);
    return *(new (&m_pData[m_count++]) _T);
  }

  template <typename... Args> _T& emplace_back(Args&&... args) {
    assert(m_count < m_capacity);
    return *(new (&m_pData[m_count++]) _T(std::forward<Args>(args)));
  }

  void resize(size_t count) {
    if (count < m_count) {
      for (size_t i = count; i < m_count; ++i)
        m_pData[i].~_T();
      m_count = count;
    } else if (count > m_count) {
      assert(count <= m_capacity);
      for (size_t i = m_count; i < count; ++i)
        new (&m_pData[i]) _T();
    }

    m_count = count;
  }

  void clear() {
    for (size_t i = 0; i < m_count; ++i)
      m_pData[i].~_T();
    m_count = 0;
  }

  void erase(size_t i) {
    assert(i < m_count);
    m_pData[i++].~_T();
    for (; i < m_count; ++i) {
      new (&m_pData[i - 1]) _T(std::move(m_pData[i]));
      m_pData[i].~_T();
    }
    --m_count;
  }

  // erases element and moves last element into removed slot
  void eraseSwap(size_t i) {
    assert(i < m_count);
    m_pData[i++].~_T();
    --m_count;
    if (i < m_count) {
      new (&m_pData[i]) _T(std::move(m_pData[m_count]));
      m_pData[m_count].~_T();
    }
  }

  size_t size() const { return m_count; }
  size_t capacity() const { return m_capacity; }

private:
  _T* m_pData;
  size_t m_count;
  size_t m_capacity;
};

#define ALTHEA_STACK_VECTOR(name, _T, capacity)                                \
  StackVector name((_T*)alloca(sizeof(_T) * (capacity)), (capacity))

} // namespace AltheaEngine