#include <Althea/Physics/Collisions.h>
#include <glm/gtc/matrix_inverse.hpp>

#include <cassert>

namespace AltheaEngine {
namespace AltheaPhysics {

static float closestPercentageAlongLineSegment(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& p) {
  glm::vec3 ab = b - a;
  glm::vec3 ap = p - a;
  float t = glm::dot(ap, ab) / glm::dot(ab, ab);
  return glm::clamp(t, 0.0f, 1.0f);
}

static glm::vec3 closestPointOnLineSegment(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& p) {
  glm::vec3 ab = b - a;
  glm::vec3 ap = p - a;
  float t = glm::dot(ap, ab) / glm::dot(ab, ab);
  t = glm::clamp(t, 0.0f, 1.0f);
  return a + ab * t;
}

/*static*/
bool Collisions::checkIntersection(
    const Capsule& C0,
    const Capsule& C1,
    CollisionResult& result) {

  glm::vec3 ab = C0.b - C0.a;
  glm::vec3 cd = C1.b - C1.a;

  glm::vec3 ac = C1.a - C0.a;
  glm::vec3 ad = C1.b - C0.a;

  float abMagSq = glm::dot(ab, ab);
  float cdMagSq = glm::dot(cd, cd);
  float abDotCd = glm::dot(ab, cd);

  float acDotAb = glm::dot(ac, ab);
  float acDotCd = glm::dot(ac, cd);

  float det = abMagSq * -cdMagSq + abDotCd * abDotCd;
  float u = 0.0f;
  float v = 0.0f;
  if (det != 0.0f) {
    det = 1.0f / det;
    u = (acDotAb * -cdMagSq + abDotCd * acDotCd) * det;
    v = (abMagSq * acDotCd - acDotAb * abDotCd) * det;
  } else {
    float u0 = 0.0f;
    float u1 = 1.0f;
    float v0 = acDotAb;
    float v1 = glm::dot(ad, ab);

    bool flip0 = false;
    bool flip1 = false;

    if (u0 > u1) {
      std::swap(u0, u1);
      flip0 = true;
    }

    if (v0 > v1) {
      std::swap(v0, v1);
      flip1 = true;
    }

    if (u0 >= v1) {
      u = flip0 ? 1.0f : 0.0f;
      v = flip1 ? 0.0f : 1.0f;
    } else if (v0 >= u1) {
      u = flip0 ? 0.0f : 1.0f;
      v = flip1 ? 1.0f : 0.0f;
    } else {
      float mid = (u0 > v0) ? (u0 + v1) * 0.5f : (v0 + u1) * 0.5f;
      u = (u0 == u1) ? 0.5f : (mid - u0) / (u1 - u0);
      v = (v0 == v1) ? 0.5f : (mid - v0) / (v1 - v0);
    }
  }

  glm::vec3 closestPoint_ab;
  glm::vec3 closestPoint_cd;

  // handle clamping
#if 1
  {
    // TODO:
    // can this be simplified??
    if (u < 0.0f || u > 1.0f) {
      u = glm::clamp(u, 0.0f, 1.0f);

      closestPoint_ab = glm::mix(C0.a, C0.b, u);
      closestPoint_cd = closestPointOnLineSegment(C1.a, C1.b, closestPoint_ab);
      closestPoint_ab = closestPointOnLineSegment(C0.a, C0.b, closestPoint_cd);
    }
    else if (v < 0.0f || v > 0.0f) {
      v = glm::clamp(v, 0.0f, 1.0f);

      closestPoint_cd = glm::mix(C1.a, C1.b, v);
      closestPoint_ab = closestPointOnLineSegment(C0.a, C0.b, closestPoint_cd);
      closestPoint_cd = closestPointOnLineSegment(C1.a, C1.b, closestPoint_ab);
    }
    else {
      closestPoint_ab = glm::mix(C0.a, C0.b, u);
      closestPoint_cd = glm::mix(C1.a, C1.b, v);
    }
  }
#else
  u = glm::clamp(u, 0.0f, 1.0f);
  v = glm::clamp(v, 0.0f, 1.0f);

  closestPoint_ab = glm::mix(C0.a, C0.b, u);
  closestPoint_cd = glm::mix(C1.a, C1.b, v);
#endif

  glm::vec3 diff = closestPoint_cd - closestPoint_ab;
  float dist2 = glm::dot(diff, diff);
  float padding = 0.1f;
  float r = C0.radius + C1.radius + padding;
  float r2 = r * r;

  if (dist2 < r2) {
    if (dist2 > 0.000001) {
      result.n = diff / sqrt(dist2);
    } else {
      // the center lines of the two capsules are directly touching
      result.n = glm::normalize(glm::cross(ab, cd));
    }

    result.ra = closestPoint_ab + result.n * C0.radius;
    result.rb = closestPoint_cd - result.n * C1.radius;

    return true;
  }

  return false;
}

} // namespace AltheaPhysics
} // namespace AltheaEngine