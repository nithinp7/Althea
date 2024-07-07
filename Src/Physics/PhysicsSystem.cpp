#include <Althea/Application.h>
#include <Althea/Physics/PhysicsSystem.h>

namespace AltheaEngine {
namespace AltheaPhysics {

PhysicsSystem::PhysicsSystem(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    SceneToGBufferPassBuilder& gBufferPassBuilder,
    GlobalHeap& heap)
    : m_dbgDrawLines(makeIntrusive<DebugDrawLines>(app)),
      m_dbgDrawCapsules(makeIntrusive<DebugDrawCapsules>(app, commandBuffer)) {
  gBufferPassBuilder.registerSubpass(m_dbgDrawLines);
  gBufferPassBuilder.registerSubpass(m_dbgDrawCapsules);
}

void PhysicsSystem::tick(float deltaTime) {
  // update rigid bodies
  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

    state.linearVelocity +=
        glm::vec3(0.0f, -m_settings.gravity, 0.0f) * deltaTime;
    state.translation += state.linearVelocity * deltaTime;

    float dAngle = glm::length(state.angularVelocity) * deltaTime;
    glm::quat dQ = glm::angleAxis(dAngle, state.angularVelocity);
    state.rotation *= dQ;
    state.rotation = glm::normalize(state.rotation);

    for (const BoundCapsule& boundCollider : rb.capsules) {
      Capsule& c = m_registeredCapsules[boundCollider.handle.colliderIdx];
      c.a = state.rotation * boundCollider.bindPose.a + state.translation;
      c.b = state.rotation * boundCollider.bindPose.b + state.translation;
    }
  }

  // floor collisions
  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

    for (const BoundCapsule& boundCollider : rb.capsules) {
      // TODO: Move this to Collisions
      const Capsule& c = m_registeredCapsules[boundCollider.handle.colliderIdx];
      glm::vec3 loc{};
      uint32_t collisions = 0;
      if (c.a.y - c.radius < m_settings.floorHeight) {
        loc += c.a;
        loc.y -= c.radius;
        ++collisions;
      }
      if (c.b.y - c.radius < m_settings.floorHeight) {
        loc += c.b;
        loc.y -= c.radius;
        ++collisions;
      }
      if (!collisions) {
        continue;
      }

      state.translation.y +=
          m_settings.floorHeight + c.radius - glm::min(c.a.y, c.b.y);

      loc /= collisions;

      glm::vec3 n(0.0f, 1.0f, 0.0f);
      glm::vec3 vLoc = getVelocityAtLocation({rbIdx}, loc);
      glm::vec3 vProj = n * glm::min(glm::dot(vLoc, n), 0.0f);
      glm::vec3 impulse = -(1.0f + m_settings.restitution) * vProj;
      applyImpulseAtLocation({rbIdx}, loc, 0.1f * impulse);
    }
  }

  // update visualization

  m_dbgDrawLines->reset();
  m_dbgDrawCapsules->reset();

  uint32_t colorRed = 0xff0000ff;
  uint32_t colorGreen = 0x00ff00ff;
  uint32_t colorBlue = 0x0000ffff;
  for (uint32_t i = 0; i < m_registeredCapsules.size(); ++i) {
    const Capsule& c = m_registeredCapsules[i];

    bool bFoundCollision = false;
    for (uint32_t j = i + 1; j < m_registeredCapsules.size(); ++j) {
      CollisionResult result{};
      if (Collisions::checkIntersection(c, m_registeredCapsules[j], result)) {
        m_dbgDrawLines->addLine(
            result.intersectionPoint - 0.5f * result.minSepTranslation,
            result.intersectionPoint + 0.5f * result.minSepTranslation,
            colorGreen);
        bFoundCollision = true;
        break;
      }
    }

    m_dbgDrawCapsules->addCapsule(
        c.a,
        c.b,
        c.radius,
        bFoundCollision ? colorRed : colorBlue);
  }
}

ColliderHandle PhysicsSystem::registerCapsuleCollider(
    const glm::vec3& a,
    const glm::vec3& b,
    float radius) {
  ColliderHandle handle;
  handle.colliderIdx = m_registeredCapsules.size();
  handle.colliderType = ColliderType::CAPSULE;
  m_registeredCapsules.push_back({a, b, radius});
  return handle;
}

RigidBodyHandle PhysicsSystem::registerRigidBody(
    const glm::vec3& translation,
    const glm::quat& rotation) {
  RigidBodyHandle h{(uint32_t)m_rigidBodies.size()};
  m_rigidBodies.emplace_back();
  RigidBodyState& initState = m_rigidBodyStates.emplace_back();
  initState.translation = translation;
  initState.rotation = rotation;
  return h;
}

void PhysicsSystem::bindCapsuleToRigidBody(
    ColliderHandle c,
    RigidBodyHandle rb) {
  assert(c.colliderType == ColliderType::CAPSULE);

  RigidBody& rigidBody = m_rigidBodies[rb.idx];
  rigidBody.capsules.push_back({c, m_registeredCapsules[c.colliderIdx]});
}

void PhysicsSystem::bakeRigidBody(RigidBodyHandle h) {
  RigidBody& rb = m_rigidBodies[h.idx];
  RigidBodyState& state = m_rigidBodyStates[h.idx];

  // TODO: generalize...
  float pointMass = 1.0f;

  // rebase capsules to new center of mass
  glm::vec3 com(0.0f);
  float mSum = 0.0f;
  for (const BoundCapsule& c : rb.capsules) {
    com += pointMass * c.bindPose.a;
    com += pointMass * c.bindPose.b;
    mSum += 2.0f * pointMass;
  }
  if (mSum > 0.0f) {
    rb.invMass = 1.0f / mSum;
    com *= rb.invMass;
  }

  for (BoundCapsule& c : rb.capsules) {
    c.bindPose.a -= com;
    c.bindPose.b -= com;
  }

  state.translation += com;

  // recompute moment of inertia
  glm::mat3 moi(0.0f);
  auto buildMoi = [&](const glm::vec3& p) {
    moi[0][0] += pointMass * (p.y * p.y + p.z * p.z);
    moi[1][0] -= pointMass * p.x * p.y;
    moi[2][0] -= pointMass * p.x * p.z;

    moi[0][1] -= pointMass * p.x * p.y;
    moi[1][1] += pointMass * (p.x * p.x + p.z * p.z);
    moi[2][1] -= pointMass * p.y * p.z;

    moi[0][2] -= pointMass * p.x * p.z;
    moi[1][2] -= pointMass * p.y * p.z;
    moi[2][2] += pointMass * (p.x * p.x + p.y * p.y);
  };

  for (const BoundCapsule& c : rb.capsules) {
    buildMoi(c.bindPose.a);
    buildMoi(c.bindPose.b);
  }

  rb.invMoi = glm::inverse(moi);
}

glm::vec3 PhysicsSystem::getVelocityAtLocation(
    RigidBodyHandle h,
    const glm::vec3& loc) const {
  const RigidBodyState& state = m_rigidBodyStates[h.idx];
  glm::vec3 r = loc - state.translation;
  return state.linearVelocity + glm::cross(state.angularVelocity, r);
}

void PhysicsSystem::applyImpulseAtLocation(
    RigidBodyHandle h,
    const glm::vec3& loc,
    const glm::vec3& impulse) {
  const RigidBody& rb = m_rigidBodies[h.idx];
  RigidBodyState& state = m_rigidBodyStates[h.idx];

  glm::vec3 r = loc - state.translation;
  float rMagSq = glm::dot(r, r);
  state.linearVelocity += rb.invMass * r * glm::dot(impulse, r) / rMagSq;
  state.angularVelocity +=
      rb.invMoi * (glm::conjugate(state.rotation) * glm::cross(r, impulse));
}

void PhysicsSystem::updateCapsule(
    ColliderHandle h,
    const glm::vec3& a,
    const glm::vec3& b) {
  assert(h.colliderType == ColliderType::CAPSULE);
  m_registeredCapsules[h.colliderIdx].a = a;
  m_registeredCapsules[h.colliderIdx].b = b;
}

void PhysicsSystem::updateCapsule(
    ColliderHandle h,
    const glm::vec3& a,
    const glm::vec3& b,
    float radius) {
  assert(h.colliderType == ColliderType::CAPSULE);
  m_registeredCapsules[h.colliderIdx].a = a;
  m_registeredCapsules[h.colliderIdx].b = b;
  m_registeredCapsules[h.colliderIdx].radius = radius;
}
} // namespace AltheaPhysics
} // namespace AltheaEngine