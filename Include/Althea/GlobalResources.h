#pragma once

#include "RayTracingResources.h"
#include "ConstantBuffer.h"
#include "DeferredRendering.h"
#include "ImageBasedLighting.h"
#include "Library.h"
#include "Model.h"
#include "PointLight.h"
#include "SingleTimeCommandBuffer.h"

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;
class GlobalHeap;

struct ALTHEA_API GlobalResourcesConstants {
  GBufferHandles gBuffer;
  IBLHandles ibl;
  RayTracingHandles raytracing;
  uint32_t shadowMapArray;
  uint32_t primitiveBuffer;
};

struct GlobalResourcesBuilder {
  RayTracingResourcesBuilder* rayTracing = nullptr;
  TextureHandle shadowMapArrayHandle{};

  char environmentMapName[256] = "NeoclassicalInterior";
};

class ALTHEA_API GlobalResources {
public:
  GlobalResources() = default;
  GlobalResources(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      const GlobalResourcesBuilder& builder);

  const IBLResources& getIBL() const { return this->_ibl; }

  const GBufferResources& getGBuffer() const { return this->_gBuffer; }
  GBufferResources& getGBuffer() { return this->_gBuffer; }

  RayTracingResources& getRayTracingResources() {
    return _rayTracingResources;
  }

  const RayTracingResources& getRayTracingResources() const {
    return _rayTracingResources;
  }

  const ConstantBuffer<GlobalResourcesConstants>& getConstants() const {
    return this->_constants;
  }
  
  ConstantBuffer<GlobalResourcesConstants>& getConstants() {
    return this->_constants;
  }

  BufferHandle getHandle() const {
    return this->_constants.getHandle();
  }

private:
  IBLResources _ibl;
  GBufferResources _gBuffer;
  RayTracingResources _rayTracingResources;

  ConstantBuffer<GlobalResourcesConstants> _constants;
};
} // namespace AltheaEngine