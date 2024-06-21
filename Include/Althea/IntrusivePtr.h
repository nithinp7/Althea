#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace AltheaEngine {
class RefCounted {
public:
  RefCounted() : m_refCount(1) {}
  virtual ~RefCounted() = default;

  RefCounted& operator=(RefCounted&& other) {
    m_refCount = other.m_refCount;
    other.m_refCount = 0;

    return *this;
  }

  RefCounted(RefCounted&& other) { *this = std::move(other); }

  uint32_t getRefCount() const { return m_refCount; }

  void incRefCount() {
    assert(m_refCount > 0);
    ++m_refCount;
  }

  void decRefCount() {
    assert(m_refCount > 0);
    if (--m_refCount == 0)
      delete this;
  }

private:
  uint32_t m_refCount = 0;
};

template <typename _T> class IntrusivePtr {

  template <typename _U> friend class IntrusivePtr;

  template <typename _U, typename... Args>
  friend IntrusivePtr<_U> makeIntrusive(Args&&... args);

public:
  IntrusivePtr() = default;

  explicit IntrusivePtr(_T* ptr) : m_ptr(ptr) {
    if (m_ptr)
      m_ptr->incRefCount();
  }

  template <typename _U>
  IntrusivePtr(const IntrusivePtr<_U>& other) : m_ptr(other.m_ptr) {
    if (other.m_ptr)
      other.m_ptr->incRefCount();
  }

  IntrusivePtr(const IntrusivePtr<_T>& other) : m_ptr(other.m_ptr) {
    if (m_ptr)
      m_ptr->incRefCount();
  }

  IntrusivePtr<_T>& operator=(const IntrusivePtr<_T>& other) {
    if (other.m_ptr)
      other.m_ptr->incRefCount();
    if (m_ptr)
      m_ptr->decRefCount();
    m_ptr = other.m_ptr;

    return *this;
  }

  _T& operator*() { return *m_ptr; }
  const _T& operator*() const { return *m_ptr; }
  _T* operator->() { return m_ptr; }
  const _T* operator->() const { return m_ptr; }

  ~IntrusivePtr() {
    if (m_ptr)
      m_ptr->decRefCount();
  }

private:
  _T* m_ptr = nullptr;
};

template <typename _T, typename... Args>
IntrusivePtr<_T> makeIntrusive(Args&&... args) {
  IntrusivePtr<_T> result;
  result.m_ptr = new _T(std::forward<Args>(args)...);
  return result;
}
} // namespace AltheaEngine