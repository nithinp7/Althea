#include "Material.h"

#include "Application.h"
#include "DefaultTextures.h"
#include "GlobalHeap.h"

#include <cstdint>

namespace AltheaEngine {
Material::Material(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    GlobalHeap& heap,
    const CesiumGltf::Model& model,
    const CesiumGltf::Material& material,
    const std::vector<Texture>& textureMap) {
  MaterialConstants constants{};

  // fill some non-zero defaults

  constants.baseColorFactor = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};

  constants.normalScale = 1.0f;
  constants.roughnessFactor = 1.0f;

  constants.occlusionStrength = 1.0f;
  constants.alphaCutoff = 0.5f;

  // If the normal map exists, we might need its UV coordinates for
  // tangent-space generation later.
  uint32_t normalMapUvIndex = 0;

  if (material.pbrMetallicRoughness) {
    const CesiumGltf::MaterialPBRMetallicRoughness& pbr =
        *material.pbrMetallicRoughness;

    if (pbr.baseColorTexture) {
      constants.baseTextureHandle =
          textureMap[pbr.baseColorTexture->index].getHandle().index;
      constants.baseTextureCoordinateIndex = pbr.baseColorTexture->texCoord;
    } else {
      constants.baseTextureHandle = GWhiteTexture1x1->getHandle().index;
      constants.baseTextureCoordinateIndex = 0;
    }
    constants.baseColorFactor = glm::vec4(
        static_cast<float>(pbr.baseColorFactor[0]),
        static_cast<float>(pbr.baseColorFactor[1]),
        static_cast<float>(pbr.baseColorFactor[2]),
        static_cast<float>(pbr.baseColorFactor[3]));

    if (pbr.metallicRoughnessTexture) {
      constants.metallicRoughnessTextureHandle =
          textureMap[pbr.metallicRoughnessTexture->index].getHandle().index;
      constants.metallicRoughnessTextureCoordinateIndex =
          pbr.metallicRoughnessTexture->texCoord;
    } else {
      constants.metallicRoughnessTextureHandle = GWhiteTexture1x1->getHandle().index;
      constants.metallicRoughnessTextureCoordinateIndex = 0;
    }

    constants.metallicFactor = static_cast<float>(pbr.metallicFactor);
    constants.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
    // TODO: reconsider default textures!! e.g., consider no texture + factors
    // metallic factor, roughness factor
  }

  if (material.normalTexture) {
    constants.normalTextureHandle = textureMap[material.normalTexture->index].getHandle().index;
    constants.normalMapTextureCoordinateIndex = material.normalTexture->texCoord;
    constants.normalScale = static_cast<float>(material.normalTexture->scale);
  }
  else {
    constants.normalTextureHandle = GNormalTexture1x1->getHandle().index;
    constants.normalMapTextureCoordinateIndex = 0.0f;
    constants.normalScale = 1.0f;
  }

  if (material.occlusionTexture) {
    constants.occlusionTextureHandle = textureMap[material.occlusionTexture->index].getHandle().index;
    constants.occlusionTextureCoordinateIndex = material.occlusionTexture->texCoord;
    constants.occlusionStrength =
        static_cast<float>(material.occlusionTexture->strength);
  }
  else 
  {
    constants.occlusionTextureHandle = GWhiteTexture1x1->getHandle().index;
    constants.occlusionTextureCoordinateIndex = 0;
    constants.occlusionStrength = 1.0f;
  }

  if (material.emissiveTexture)
  {
    constants.emissiveTextureHandle = textureMap[material.emissiveTexture->index].getHandle().index;
    constants.emissiveTextureCoordinateIndex = material.emissiveTexture->texCoord;
  }
  else
  {
    constants.emissiveTextureHandle = GBlackTexture1x1->getHandle().index;
    constants.emissiveTextureCoordinateIndex = 0;
  }

  constants.emissiveFactor = glm::vec4(
      static_cast<float>(material.emissiveFactor[0]),
      static_cast<float>(material.emissiveFactor[1]),
      static_cast<float>(material.emissiveFactor[2]),
      1.0f);

  constants.alphaCutoff = static_cast<float>(material.alphaCutoff);

  m_constants = ConstantBuffer<MaterialConstants>(app, commandBuffer, constants);
  m_constants.registerToHeap(heap);
}
} // namespace AltheaEngine