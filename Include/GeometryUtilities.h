#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "mikktspace.h"

namespace AltheaEngine {

class GeometryUtilities {
public:
  template <typename TVertexIn, typename TVertexOut, typename TVertexConverter, typename TIndex>
  static std::vector<TVertexOut> duplicateVertices(
      const std::vector<TVertexIn>& originalVertices, const std::vector<TIndex>& indices, TVertexConverter converter) {
    std::vector<TVertex> result;
    result.resize(indices.size());

    for (size_t i = 0; i < indices.size(); ++i) {
      result[i] = converter(originalVertices[static_cast<size_t>(indices[i])]);
    }

    return result;
  }

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
  template<typename TVertex>
  static void computeTangentSpace(std::vector<TVertex>& vertices) {
    SMikkTSpaceInterface MikkTInterface{};
    MikkTInterface.m_getNormal = _mikkGetNormal<TVertex>;
    MikkTInterface.m_getNumFaces = _mikkGetNumFaces<TVertex>;
    MikkTInterface.m_getNumVerticesOfFace = _mikkGetNumVertsOfFace<TVertex>;
    MikkTInterface.m_getPosition = _mikkGetPosition<TVertex>;
    MikkTInterface.m_getTexCoord = _mikkGetTexCoord<TVertex>;
    MikkTInterface.m_setTSpaceBasic = _mikkSetTSpaceBasic<TVertex>;
    MikkTInterface.m_setTSpace = nullptr;

    SMikkTSpaceContext MikkTContext{};
    MikkTContext.m_pInterface = &MikkTInterface;
    MikkTContext.m_pUserData = (void*)(&vertices);
    // MikkTContext.m_bIgnoreDegenerates = false;
    genTangSpaceDefault(&MikkTContext);
  }

private:
  // Implementations of MikkTSpace interface
  template<typename TVertex>
  static int _mikkGetNumFaces(const SMikkTSpaceContext* Context) {
    std::vector<TVertex>& vertices = 
        *reinterpret_cast<std::vector<TVertex>*>(Context->m_pUserData);
    vertices.size() / 3;
  }
  
  template<typename TVertex>
  static int
  _mikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx) {
    std::vector<TVertex>& vertices = 
        *reinterpret_cast<std::vector<TVertex>*>(Context->m_pUserData);
    return FaceIdx < (vertices.size() / 3) ? 3 : 0;
  }
  
  template<typename TVertex>
  static void _mikkGetPosition(
      const SMikkTSpaceContext* Context,
      float Position[3],
      const int FaceIdx,
      const int VertIdx) {
    std::vector<TVertex>& vertices = 
        *reinterpret_cast<std::vector<TVertex>*>(Context->m_pUserData);
    const glm::vec3& position = vertices[3 * FaceIdx + VertIdx].position;
    Position[0] = position.x;
    Position[1] = position.y;
    Position[2] = position.z;
  }
      
  template<typename TVertex>
  static void _mikkGetNormal(
      const SMikkTSpaceContext* Context,
      float Normal[3],
      const int FaceIdx,
      const int VertIdx) {
    std::vector<TVertex>& vertices = 
        *reinterpret_cast<std::vector<TVertex>*>(Context->m_pUserData);
    const glm::vec3& normal = vertices[3 * FaceIdx + VertIdx].normal;
    Normal[0] = normal.x;
    Normal[1] = normal.y;
    Normal[2] = normal.z;
  }

  template<typename TVertex>
  static void _mikkGetTexCoord(
      const SMikkTSpaceContext* Context,
      float UV[2],
      const int FaceIdx,
      const int VertIdx) {
    std::vector<TVertex>& vertices = 
        *reinterpret_cast<std::vector<TVertex>*>(Context->m_pUserData);
    const glm::vec2& uv0 = vertices[3 * FaceIdx + VertIdx].uvs[0];
    UV[0] = uv0.x;
    UV[1] = uv0.y;
  }
      
  template<typename TVertex>
  static void _mikkSetTSpaceBasic(
      const SMikkTSpaceContext* Context,
      const float Tangent[3],
      const float BitangentSign,
      const int FaceIdx,
      const int VertIdx) {
    
    std::vector<TVertex>& vertices = 
        *reinterpret_cast<std::vector<TVertex>*>(Context->m_pUserData);
    TVertex& vertex = vertices[3 * FaceIdx + VertIdx];
    vertex.tangent.x = Tangent[0];
    vertex.tangent.y = Tangent[1];
    vertex.tangent.z = Tangent[2];

    vertex.bitangent = 
        BitangentSign * 
        glm::cross(vertex.normal, vertex.tangent);
  }
};
} // namespace AltheaEngine