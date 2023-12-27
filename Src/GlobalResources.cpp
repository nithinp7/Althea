#include "GlobalResources.glsl"

#include "Application.h"
#include "GlobalHeap.h"

namespace AltheaEngine {

GlobalResources::GlobalResources(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    BufferHandle primitiveConstantBuffer,
    BufferHandle lightConstantBuffer)
    : _ibl(app, commandBuffer, "NeoclassicalInterior"), // TODO: paramaterize
      _gBuffer(app) {

  this->_gBuffer.registerToHeap(heap);
  this->_ibl.registerToHeap(heap);

  GlobalResourcesConstants constants{};
  constants.gBuffer = this->_gBuffer.getHandes();
  constants.ibl = this->_ibl.getHandes();
  constants.lightBuffer = lightConstantBuffer.index;
  constants.primitiveBuffer = primitiveConstantBuffer.index;
}
} // namespace AltheaEngine