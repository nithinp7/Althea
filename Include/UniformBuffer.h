#pragma once

#include <vulkan/vulkan.h>

#include <memory>

namespace AltheaEngine {
class Application;

// TODO: Make CPU-copy of uniform buffer optional.

template <typename TUniforms> class UniformBuffer {
public:
  UniformBuffer() = default;

  UniformBuffer(UniformBuffer&& rhs)
      : _device(rhs._device), _buffer(rhs._buffer), _memory(rhs._memory) {
    rhs._device = VK_NULL_HANDLE;
    rhs._buffer = VK_NULL_HANDLE;
    rhs._memory = VK_NULL_HANDLE;
  }

  UniformBuffer& operator=(UniformBuffer&& rhs) {
    this->_device = rhs._device;
    this->_buffer = rhs._buffer;
    this->_memory = rhs._memory;

    rhs._device = VK_NULL_HANDLE;
    rhs._buffer = VK_NULL_HANDLE;
    rhs._memory = VK_NULL_HANDLE;
  }

  UniformBuffer(const UniformBuffer& rhs) = delete;
  UniformBuffer& operator=(const UniformBuffer& rhs) = delete;

  UniformBuffer(const Application& app) : _device(app.getDevice()) {
    app.createUniformBuffer(sizeof(TUniforms), this->_buffer, this->_memory);
  }

  UniformBuffer(const Application& app, const TUniforms& uniforms)
      : _device(app.getDevice()) {
    app.createUniformBuffer(sizeof(TUniforms), this->_buffer, this->_memory);

    updateUniforms(uniforms);
  }

  ~UniformBuffer() {
    if (this->_device) {
      vkDestroyBuffer(this->_device, this->_buffer, nullptr);
      vkFreeMemory(this->_device, this->_memory, nullptr);
    }
  }

  // TODO: TUniforms& getUniforms()
  // TODO: syncBuffer()

  void updateUniforms(const TUniforms& uniforms) {
    this->_uniforms = uniforms;

    void* dst;
    vkMapMemory(this->_device, this->_memory, 0, sizeof(TUniforms), 0, &dst);
    memcpy(dst, &this->_uniforms, sizeof(TUniforms));
    vkUnmapMemory(this->_device, this->_memory);
  }

  const TUniforms& getUniforms() const { return this->_uniforms; }

  VkBuffer getBuffer() const { return this->_buffer; }

  size_t getSize() const { return sizeof(TUniforms); }

private:
  TUniforms _uniforms;

  VkDevice _device = VK_NULL_HANDLE;

  VkBuffer _buffer;
  VkDeviceMemory _memory;
};

} // namespace AltheaEngine