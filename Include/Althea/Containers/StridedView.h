#pragma once

#include "SimpleIterator.h"
#include "StackVector.h"

#include <memory>
#include <vector>

namespace AltheaEngine {

template <typename _TView> struct StridedView {
  typedef ConstStridedIterator<_TView> const_iterator;

  const_iterator begin() const { return {m_pStart + m_offset, m_stride}; }
  const_iterator end() const {
    return {m_pStart + m_offset + m_stride * m_count, m_stride};
  }

  const _TView* begin_ptr() const {
    return reinterpret_cast<const _TView*>(m_pStart + m_offset);
  }
  const _TView* end_ptr() const {
    return reinterpret_cast<const _TView*>(
        m_pStart + m_offset + m_stride * m_count);
  }

  template <typename _TStruct>
  StridedView(const _TStruct* pStart, size_t offset, size_t count)
      : m_pStart(reinterpret_cast<const std::byte*>(pStart)),
        m_offset(offset),
        m_stride(sizeof(_TStruct)),
        m_count(count) {}

  StridedView(const StackVector<_TView>& vec)
      : m_pStart(reinterpret_cast<const std::byte*>(&vec[0])),
        m_offset(0),
        m_stride(sizeof(_TView)),
        m_count(vec.size()) {}

  StridedView(const std::vector<_TView>& vec)
      : m_pStart(reinterpret_cast<const std::byte*>(vec.data())),
        m_offset(0),
        m_stride(sizeof(_TView)),
        m_count(vec.size()) {}

  const _TView& operator[](size_t idx) const {
    const std::byte* ptr = m_pStart + m_offset + idx * m_stride;
    return *reinterpret_cast<const _TView*>(ptr);
  }

  size_t getCount() const { return m_count; }

private:
  const std::byte* m_pStart;
  size_t m_offset;
  size_t m_stride;
  size_t m_count;
};
} // namespace AltheaEngine