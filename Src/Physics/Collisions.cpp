#include <Althea/Physics/Collisions.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <cassert>

namespace AltheaEngine {
namespace AltheaPhysics {

/*static*/
bool Collisions::checkIntersection(
    const Capsule& C0,
    const Capsule& C1,
    CollisionResult& result) {  

  // Find closest pair of points between the two lines

  glm::vec3 ab = C0.b - C0.a;
  glm::vec3 cd = C1.b - C1.a;
  
  // 3 rows by 2 columns
  glm::mat2x3 A(ab, -cd);
  glm::vec3 B = C0.a - C1.a;

  glm::mat3x2 At = glm::transpose(A);
  glm::mat2 AtA = At * A; // TODO: Handle degenerate ranks

  // pseudo-inverse
  glm::vec2 x = -glm::inverse(AtA) * At * B;

  // clamp within the height of the actual capsules
  float u = glm::clamp(x.x, 0.0f, 1.0f);
  float v = glm::clamp(x.y, 0.0f, 1.0f);

  glm::vec3 closestPoint_ab = glm::mix(C0.a, C0.b, u);
  glm::vec3 closestPoint_cd = glm::mix(C1.a, C1.b, v);

  glm::vec3 diff = closestPoint_cd - closestPoint_ab;
  float dist2 = glm::dot(diff, diff);
  float r = C0.radius + C1.radius;
  float r2 = r * r;
  
  if (dist2 < r2) {
    diff /= sqrt(dist2);

    glm::vec3 closestPointSurface_C0 = closestPoint_ab + diff * C0.radius;
    glm::vec3 closestPointSurface_C1 = closestPoint_cd - diff * C1.radius;
    
    result.minSepTranslation = closestPointSurface_C1 - closestPointSurface_C0;
    result.intersectionPoint = 0.5f * (closestPointSurface_C0 + closestPointSurface_C1);

    return true; 
  }

  return false;
}
} // namespace AltheaPhysics
} // namespace AltheaEngine