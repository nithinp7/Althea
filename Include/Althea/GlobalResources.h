#pragma once

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
  uint32_t shadowMapArray;
  uint32_t primitiveBuffer;
};

class ALTHEA_API GlobalResources {
public:
  GlobalResources() = default;
  GlobalResources(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      ImageHandle shadowMapArrayHandle,
      BufferHandle primitiveConstantBuffer);

  const IBLResources& getIBL() const { return this->_ibl; }

  const GBufferResources& getGBuffer() const { return this->_gBuffer; }
  GBufferResources& getGBuffer() { return this->_gBuffer; }

  const ConstantBuffer<GlobalResourcesConstants>& getConstants() const {
    return this->_constants;
  }

  BufferHandle getHandle() const {
    return this->_constants.getHandle();
  }

private:
  IBLResources _ibl;
  GBufferResources _gBuffer;

  ConstantBuffer<GlobalResourcesConstants> _constants;
};
} // namespace AltheaEngine