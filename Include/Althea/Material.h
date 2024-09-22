#pragma once

#include "BindlessHandle.h"
#include "Common/InstanceDataCommon.h"
#include "ConstantBuffer.h"
#include "Texture.h"

#include <CesiumGltf/Material.h>

#include <memory>
#include <vector>

namespace AltheaEngine {

class Application;
class GlobalHeap;
class SIngleTimeCommandBuffer;

class ALTHEA_API Material {
public:
  Material() = default;
  Material(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      GlobalHeap& heap,
      const CesiumGltf::Model& model,
      const CesiumGltf::Material& material,
      const std::vector<Texture>& textureMap);

  BufferHandle getHandle() const { return m_constants.getHandle(); }

  int getNormalTextureCoordinateIndex() const {
    return m_constants.getConstants().normalMapTextureCoordinateIndex;
  }

private:
  ConstantBuffer<MaterialConstants> m_constants;
};
} // namespace AltheaEngine