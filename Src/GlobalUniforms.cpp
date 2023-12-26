#include "GlobalUniforms.h"

#include "Application.h"

namespace AltheaEngine {
GlobalUniformsResource::GlobalUniformsResource(
    const Application& app,
    GlobalHeap& heap) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    this->_indices[i] = heap.registerBuffer();
    this->_buffers[i] = UniformBuffer<GlobalUniforms>(app);
    heap.updateUniformBuffer(
        this->_indices[i],
        this->_buffers[i].getAllocation().getBuffer(),
        0,
        this->_buffers[i].getSize());
  }
}
} // namespace AltheaEngine