#pragma once

#include <glm/glm.hpp>
#include <glm/matrix.hpp>

namespace AltheaEngine {
namespace AltheaPhysics {

// Each collider type has a canonical shape given an identity
// transform. So a transformed collider can always be fully
// specified by a transformation and collider type.
enum class ColliderType { FLOOR = 0, CAPSULE, BOX, SPHERE, COUNT };

// TODO: Impl box / sphere / convex-volumes
struct Capsule {
  glm::vec3 a;
  glm::vec3 b;
  float radius;
};

struct CollisionResult {
  glm::vec3 intersectionPoint{};
  glm::vec3 minSepTranslation{};
};
class Collisions {
public:
  static bool checkIntersection(
      const Capsule& a,
      const Capsule& b,
      CollisionResult& result);
};
} // namespace AltheaPhysics
} // namespace AltheaEngine