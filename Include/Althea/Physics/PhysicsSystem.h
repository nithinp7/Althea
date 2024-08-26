#pragma once

#include "RigidBody.h"

#include <Althea/Containers/StackVector.h>
#include <Althea/Containers/StridedView.h>
#include <Althea/Debug/DebugDraw.h>
#include <Althea/DeferredRendering.h>
#include <Althea/GlobalHeap.h>
#include <Althea/Gui.h>
#include <Althea/IntrusivePtr.h>
#include <Althea/Physics/Collisions.h>
#include <Althea/SingleTimeCommandBuffer.h>

#include <cstdint>
#include <vector>

namespace AltheaEngine {
class Application;

namespace AltheaPhysics {

struct PhysicsWorldSettings {
  float gravity = 20.0f;
  float restitution = 0.0f;
  float staticFriction = 0.f;
  float dynamicFriction = 0.1f;
  float angularDamping = 2.0f;
  float linearDamping = 2.0;
  float floorHeight = -8.0f;

  float maxSpeed = 50.0f;
  float maxAngularSpeed = 1.0f;

  int timeSubsteps = 10;
  int positionIterations = 1;

  // DEBUG STUFF
  bool enableVelocityUpdate = true;
  bool enableDynamicCollisions = true;
  bool debugDrawCapsules = true;
  bool wireframeCapsules = false;
  bool debugDrawVelocities = true;
  bool debugDrawCollisions = true;
};

struct PositionConstraint {
  glm::vec3 rA;
  uint32_t rbAIdx;
  glm::vec3 rB;
  uint32_t rbBIdx;
  float lambda;
};

class PhysicsSystem {
public:
  PhysicsSystem() = default;

  PhysicsSystem(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      SceneToGBufferPassBuilder& gBufferPassBuilder,
      GlobalHeap& heap);

  void tick(float deltaTime);

  ColliderHandle
  registerCapsuleCollider(const glm::vec3& a, const glm::vec3& b, float radius);

  RigidBodyHandle
  registerRigidBody(const glm::vec3& translation, const glm::quat& rotation);

  void bindCapsuleToRigidBody(ColliderHandle c, RigidBodyHandle rb);

  void updateCapsule(ColliderHandle h, const glm::vec3& a, const glm::vec3& b);
  void updateCapsule(
      ColliderHandle h,
      const glm::vec3& a,
      const glm::vec3& b,
      float radius);

  void bakeRigidBody(RigidBodyHandle h);

  uint32_t getCapsuleCount() const { return m_registeredCapsules.size(); }

  const Capsule& getCapsule(uint32_t idx) const {
    return m_registeredCapsules[idx];
  }

  uint32_t getRigidBodyCount() const { return m_rigidBodies.size(); }

  const RigidBody& getRigidBody(uint32_t idx) const {
    return m_rigidBodies[idx];
  }

  const RigidBodyState& getRigidBodyState(uint32_t idx) const {
    return m_rigidBodyStates[idx];
  }

  PhysicsWorldSettings& getSettings() { return m_settings; }
  const PhysicsWorldSettings& getSettings() const { return m_settings; }

  void forceUpdateCapsules();
  void forceDebugDraw(float deltaTime);

  void debugSaveToFile(const char* filename);
  void debugLoadFromFile(const char* filename);

private:
  void xpbd_integrateState(float h);

  struct StaticCollision {
    glm::vec3 nStatic;
    float lambdaN;
    glm::vec3 rStatic;
    float lambdaT;
    glm::vec3 rRB;
    uint32_t rigidBodyIdx;
  };

  struct DynamicCollision {
    glm::vec3 n;
    float lambdaN;
    glm::vec3 rA;
    float lambdaT;
    glm::vec3 rB;
    uint32_t rbAIdx;
    uint32_t rbBIdx;
  };
  void xpbd_findCollisions();
  void xpbd_solveCollisionPositions();

  void xpbd_predictVelocities(float h);
  void xpbd_solveCollisionVelocities(float h);

  void computeMomentOfInertia(uint32_t rbIdx, glm::mat3& I, glm::mat3& I_inv) const;

  void debugDraw(float deltaTime);

  std::vector<BoundCapsule> m_boundCapsules;
  std::vector<Capsule> m_registeredCapsules;
  std::vector<uint32_t> m_capsuleOwners;

  std::vector<RigidBody> m_rigidBodies;
  std::vector<RigidBodyState> m_rigidBodyStates;

  std::vector<StaticCollision> m_staticCollisions;
  std::vector<DynamicCollision> m_dynamicCollisions;

  std::vector<PositionConstraint> m_positionConstraints;

  IntrusivePtr<DebugDrawLines> m_dbgDrawLines;
  IntrusivePtr<DebugDrawCapsules> m_dbgDrawCapsules;
  IntrusivePtr<DebugDrawCapsules> m_dbgDrawCapsulesWireframe;

  PhysicsWorldSettings m_settings{};
};
} // namespace AltheaPhysics
} // namespace AltheaEngine