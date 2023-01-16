#pragma once

#include <vulkan/vulkan.h>

namespace AltheaEngine {

// TODO: do some type-trait validation on if THandle is actually a vulkan handle
// type

template <typename THandle, typename TDeleter> class UniqueVkHandle {
public:
  UniqueVkHandle() : _device(VK_NULL_HANDLE), _handle(VK_NULL_HANDLE) {}

  UniqueVkHandle(VkDevice device, THandle handle, TDeleter&& deleter = {})
      : _device(device), _handle(handle), _deleter(std::move(deleter)) {}

  // Move-only semantics
  UniqueVkHandle(UniqueVkHandle&& rhs)
      : _device(rhs._device), _handle(rhs._handle) {
    rhs._device = VK_NULL_HANDLE;
    rhs._handle = VK_NULL_HANDLE;
  }

  UniqueVkHandle& operator=(UniqueVkHandle&& rhs) {
    if (this->_handle != VK_NULL_HANDLE) {
      // TODO: Should this be considered an error instead?
      this->destroy();
    }

    this->_device = rhs._device;
    this->_handle = rhs._handle;

    rhs._device = VK_NULL_HANDLE;
    rhs._handle = VK_NULL_HANDLE;

    return *this;
  }

  UniqueVkHandle(const UniqueVkHandle& rhs) = delete;
  UniqueVkHandle& operator=(const UniqueVkHandle& rhs) = delete;

  void set(VkDevice device, THandle handle) {
    if (this->_handle != VK_NULL_HANDLE) {
      // TODO: Should this be considered an error instead?
      this->destroy();
    }

    this->_device = device;
    this->_handle = handle;
  }

  ~UniqueVkHandle() {
    if (this->_handle != VK_NULL_HANDLE) {
      this->destroy();
    }
  }

  // Not safe to call more than once.
  void destroy() {
    this->_deleter(this->_device, this->_handle);

    this->_device = VK_NULL_HANDLE;
    this->_handle = VK_NULL_HANDLE;
  }

  // Transparently converts to the underlying handle type
  operator THandle() const { return this->_handle; }

  operator bool() const { return this->_handle != VK_NULL_HANDLE; }

private:
  VkDevice _device;
  THandle _handle;
  TDeleter _deleter;
};
} // namespace AltheaEngine