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
  float gravity = 10.0f;
  float restitution = 0.8f;
  float frictionCoeff = 0.8f;
  float angularDamping = 2.0f;
  float linearDamping = 2.0;
  float floorHeight = -8.0f;

  float maxSpeed = 50.0f;
  float maxAngularSpeed = 1.0f;

  float SI_bias = 0.0f;
  int SI_iters = 1;

  // DEBUG STUFF
  bool enableVelocityUpdate = true;
  bool debugDrawCapsules = true;
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

  glm::vec3
  getVelocityAtLocation(RigidBodyHandle h, const glm::vec3& loc) const;
  void computeImpulseVelocityAtLocation(
      RigidBodyHandle h,
      const glm::vec3& loc,
      const glm::vec3& impulse,
      glm::vec3& dVLin,
      glm::vec3& dVAng);
  void applyImpulseAtLocation(
      RigidBodyHandle h,
      const glm::vec3& loc,
      const glm::vec3& impulse);

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
private:
  void xpbd_applyForces(float deltaTime);

  struct StaticCollision {
    glm::vec3 nStatic;
    float C;
    glm::vec3 rStatic;
    glm::vec3 rRB;
    uint32_t rigidBodyIdx;
  };

  struct DynamicCollision {

  };
  void xpbd_findCollisions(StackVector<StaticCollision>& staticCollisions, StackVector<DynamicCollision>& dynamicCollisions);
  void xpbd_solveCollisionPositions(StridedView<StaticCollision> staticCollisions, StridedView<DynamicCollision> dynamicCollisions);

  void xpbd_predictVelocities(float deltaTime);
  void xpbd_solveCollisionVelocities(float deltaTime, StridedView<StaticCollision> staticCollisions, StridedView<DynamicCollision> dynamicCollisions);

  void debugDrawCapsules();

  std::vector<Capsule> m_registeredCapsules;

  std::vector<RigidBody> m_rigidBodies;
  std::vector<RigidBodyState> m_rigidBodyStates;

  IntrusivePtr<DebugDrawLines> m_dbgDrawLines;
  IntrusivePtr<DebugDrawCapsules> m_dbgDrawCapsules;

  PhysicsWorldSettings m_settings{};
};
} // namespace AltheaPhysics
} // namespace AltheaEngine