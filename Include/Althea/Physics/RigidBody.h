#pragma once

#include "Collisions.h"

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <vector>

namespace AltheaEngine {
namespace AltheaPhysics {

struct BoundCapsule {
  ColliderHandle handle;
  Capsule bindPose;
};

struct RigidBody {
  glm::mat3 invMoi;
  float invMass;
  std::vector<BoundCapsule> capsules;
};

struct RigidBodyHandle {
  uint32_t idx;
};

struct RigidBodyState {
  glm::vec3 translation{};
  glm::quat rotation{};
  glm::vec3 linearVelocity{};
  glm::vec3 angularVelocity{};
};
} // namespace AltheaPhysics
} // namespace AltheaEngine