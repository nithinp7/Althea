#include "GlobalResources.h"

#include "Application.h"
#include "GlobalHeap.h"

namespace AltheaEngine {

GlobalResources::GlobalResources(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    const GlobalResourcesBuilder& builder) {

  this->_gBuffer = GBufferResources(app);
  this->_gBuffer.registerToHeap(heap);

  // Paramaterize this
  this->_ibl = ImageBasedLighting::createResources(
      app,
      commandBuffer,
      "NeoclassicalInterior");
  this->_ibl.registerToHeap(heap);

  GlobalResourcesConstants constants{};
  constants.gBuffer = this->_gBuffer.getHandles();
  constants.ibl = this->_ibl.getHandles();
  constants.shadowMapArray = builder.shadowMapArrayHandle.index;

  if (builder.rayTracing)
  {
    _rayTracingResources = RayTracingResources(app, commandBuffer, heap, *builder.rayTracing);
    constants.raytracing = _rayTracingResources.getHandles();
  }

  this->_constants =
      ConstantBuffer<GlobalResourcesConstants>(app, commandBuffer, constants);
  this->_constants.registerToHeap(heap);
}
} // namespace AltheaEngine