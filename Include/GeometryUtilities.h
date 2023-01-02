#pragma once

#include "mikktspace.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>


namespace AltheaEngine {

namespace {
template <typename TVertex> struct VerticesAndUvIndex {
  std::vector<TVertex>* vertices;
  uint32_t uvIndex;
};
} // namespace

class GeometryUtilities {
public:
  /**
   * @brief Fill in normals for these vertices. Assumes a non-indexed, triangle
   * mesh.
   *
   * @tparam TVertex
   * @param vertices
   */
  template <typename TVertex>
  static void computeFlatNormals(std::vector<TVertex>& vertices) {
    for (size_t i = 0; i < vertices.size() / 3; ++i) {
      // Assumes the winding is in a positive (clockwise) order
      // about the normal, in a right-handed coordinate system.
      TVertex& a = vertices[3 * i + 0];
      TVertex& b = vertices[3 * i + 1];
      TVertex& c = vertices[3 * i + 2];

      glm::vec3 ab = b.position - a.position;
      glm::vec3 ac = c.position - a.position;

      glm::vec3 normal = glm::normalize(glm::cross(ab, ac));
      a.normal = normal;
      b.normal = normal;
      c.normal = normal;
    }
  }

  // TODO: abstract the Vertex interface instead of using a templated type here
  template <typename TVertex>
  static void
  computeTangentSpace(std::vector<TVertex>& vertices, uint32_t uvIndex) {
    SMikkTSpaceInterface MikkTInterface{};
    MikkTInterface.m_getNormal = _mikkGetNormal<TVertex>;
    MikkTInterface.m_getNumFaces = _mikkGetNumFaces<TVertex>;
    MikkTInterface.m_getNumVerticesOfFace = _mikkGetNumVertsOfFace<TVertex>;
    MikkTInterface.m_getPosition = _mikkGetPosition<TVertex>;
    MikkTInterface.m_getTexCoord = _mikkGetTexCoord<TVertex>;
    MikkTInterface.m_setTSpaceBasic = _mikkSetTSpaceBasic<TVertex>;
    MikkTInterface.m_setTSpace = nullptr;

    VerticesAndUvIndex<TVertex> vertsAndUvIndex{&vertices, uvIndex};

    SMikkTSpaceContext MikkTContext{};
    MikkTContext.m_pInterface = &MikkTInterface;
    MikkTContext.m_pUserData = (void*)(&vertsAndUvIndex);
    // MikkTContext.m_bIgnoreDegenerates = false;
    genTangSpaceDefault(&MikkTContext);
  }

private:
  // Implementations of MikkTSpace interface
  template <typename TVertex>
  static int _mikkGetNumFaces(const SMikkTSpaceContext* Context) {
    std::vector<TVertex>& vertices =
        *reinterpret_cast<VerticesAndUvIndex<TVertex>*>(Context->m_pUserData)
             ->vertices;
    return static_cast<int>(vertices.size()) / 3;
  }

  template <typename TVertex>
  static int
  _mikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx) {
    std::vector<TVertex>& vertices =
        *reinterpret_cast<VerticesAndUvIndex<TVertex>*>(Context->m_pUserData)
             ->vertices;
    return FaceIdx < (vertices.size() / 3) ? 3 : 0;
  }

  template <typename TVertex>
  static void _mikkGetPosition(
      const SMikkTSpaceContext* Context,
      float Position[3],
      const int FaceIdx,
      const int VertIdx) {
    std::vector<TVertex>& vertices =
        *reinterpret_cast<VerticesAndUvIndex<TVertex>*>(Context->m_pUserData)
             ->vertices;
    const glm::vec3& position = vertices[3 * FaceIdx + VertIdx].position;
    Position[0] = position.x;
    Position[1] = position.y;
    Position[2] = position.z;
  }

  template <typename TVertex>
  static void _mikkGetNormal(
      const SMikkTSpaceContext* Context,
      float Normal[3],
      const int FaceIdx,
      const int VertIdx) {
    std::vector<TVertex>& vertices =
        *reinterpret_cast<VerticesAndUvIndex<TVertex>*>(Context->m_pUserData)
             ->vertices;
    const glm::vec3& normal = vertices[3 * FaceIdx + VertIdx].normal;
    Normal[0] = normal.x;
    Normal[1] = normal.y;
    Normal[2] = normal.z;
  }

  template <typename TVertex>
  static void _mikkGetTexCoord(
      const SMikkTSpaceContext* Context,
      float UV[2],
      const int FaceIdx,
      const int VertIdx) {
    VerticesAndUvIndex<TVertex>& vertsAndUvIndex =
        *reinterpret_cast<VerticesAndUvIndex<TVertex>*>(Context->m_pUserData);
    std::vector<TVertex>& vertices = *vertsAndUvIndex.vertices;

    const glm::vec2& uv0 =
        vertices[3 * FaceIdx + VertIdx].uvs[vertsAndUvIndex.uvIndex];
    UV[0] = uv0.x;
    UV[1] = uv0.y;
  }

  template <typename TVertex>
  static void _mikkSetTSpaceBasic(
      const SMikkTSpaceContext* Context,
      const float Tangent[3],
      const float BitangentSign,
      const int FaceIdx,
      const int VertIdx) {

    std::vector<TVertex>& vertices =
        *reinterpret_cast<VerticesAndUvIndex<TVertex>*>(Context->m_pUserData)
             ->vertices;
    TVertex& vertex = vertices[3 * FaceIdx + VertIdx];
    vertex.tangent.x = Tangent[0];
    vertex.tangent.y = Tangent[1];
    vertex.tangent.z = Tangent[2];

    vertex.bitangent =
        BitangentSign * glm::cross(vertex.normal, vertex.tangent);
  }
};
} // namespace AltheaEngine