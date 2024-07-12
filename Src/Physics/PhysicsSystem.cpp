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

#define XPBD

void PhysicsSystem::tick(float deltaTime) {
  // update rigid bodies

  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

#ifdef XPBD
    state.linearVelocity =
        (state.translation - state.prevTranslation) / deltaTime;
    state.prevTranslation = state.translation;

    glm::quat dQ =
        glm::normalize(glm::conjugate(state.prevRotation) * state.rotation);
    state.angularVelocity =
        state.prevRotation * (glm::angle(dQ) / deltaTime * glm::axis(dQ));
    state.prevRotation = state.rotation;
#endif

    
    // state.linearVelocity *= m_settings.linearDamping;
    // state.angularVelocity *= m_settings.angularDamping;

    state.linearVelocity +=
        glm::vec3(0.0f, -m_settings.gravity, 0.0f) * deltaTime;
    float speed = glm::length(state.linearVelocity);
    if (speed > m_settings.maxSpeed) {
      state.linearVelocity *= m_settings.maxSpeed / speed;
    }
    
    state.translation += state.linearVelocity * deltaTime;

    float angularSpeed = glm::length(state.angularVelocity);
    if (angularSpeed > 0.001f) {
      if (angularSpeed > m_settings.maxAngularSpeed) {
        state.angularVelocity *= m_settings.maxAngularSpeed / angularSpeed;
        angularSpeed = m_settings.maxAngularSpeed;
      }

      glm::quat dQ = glm::angleAxis(
          angularSpeed * deltaTime,
          state.angularVelocity / angularSpeed);
      state.rotation = dQ * state.rotation;
      state.rotation = glm::normalize(state.rotation);
    }

    for (const BoundCapsule& boundCollider : rb.capsules) {
      Capsule& c = m_registeredCapsules[boundCollider.handle.colliderIdx];
      c.a = state.rotation * boundCollider.bindPose.a + state.translation;
      c.b = state.rotation * boundCollider.bindPose.b + state.translation;
    }
  }

  // floor collisions
  for (uint32_t siIter = 0; siIter < m_settings.SI_iters; ++siIter) {
    for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
      const RigidBody& rb = m_rigidBodies[rbIdx];
      RigidBodyState& state = m_rigidBodyStates[rbIdx];

      glm::vec3 dVLin(0.0f);
      glm::vec3 dVAng(0.0f);

      for (const BoundCapsule& boundCollider : rb.capsules) {
        // TODO: Move this to Collisions
        const Capsule& c =
            m_registeredCapsules[boundCollider.handle.colliderIdx];
        glm::vec3 loc{};
        uint32_t collisions = 0;
        float minY = m_settings.floorHeight;
        if (c.a.y - c.radius < m_settings.floorHeight) {
          loc += c.a;
          loc.y -= c.radius;
          minY = glm::min(minY, c.a.y - c.radius);
          ++collisions;
        }
        if (c.b.y - c.radius < m_settings.floorHeight) {
          loc += c.b;
          loc.y -= c.radius;
          minY = glm::min(minY, c.b.y - c.radius);
          ++collisions;
        }
        if (!collisions) {
          continue;
        }

        loc /= collisions;

        float pen = glm::max(m_settings.floorHeight - loc.y, 0.0f);

#ifdef XPBD
        if (pen == 0.0f)
          continue;

        glm::vec3 n(0.0f, 1.0f, 0.0f);
        glm::vec3 r = loc - state.translation;

        glm::quat qc = glm::conjugate(state.rotation);

        glm::vec3 rxn = qc * glm::cross(r, n);
        
        // effective mass at r
        float w = rb.invMass + glm::dot(rxn, rb.invMoi * rxn);
        
        float stiffness = 1.0f * deltaTime;
        glm::vec3 P = stiffness * pen / w * n;

        float rMagSq = glm::dot(r, r);
        state.translation += rb.invMass * r * glm::dot(P, r) / rMagSq;

        // linearized rotation update
        state.rotation += 0.5f * glm::quat(0.0f, rb.invMoi * glm::cross(r, P)) * state.rotation;
        state.rotation = glm::normalize(state.rotation);
#else
        state.translation.y += 0.1f * pen;
        glm::vec3 n(0.0f, 1.0f, 0.0f);
        glm::vec3 r = loc - state.translation;

        // TODO: Look into how this works (Erin Catto: Sequential Impulses)
        glm::quat qc = glm::conjugate(state.rotation);

        float kn =
            rb.invMass + glm::dot(
                             rb.invMoi * (qc * glm::cross(glm::cross(r, n), r)),
                             qc * n);

        glm::vec3 vLoc = getVelocityAtLocation({rbIdx}, loc);
        float vDotN = glm::dot(vLoc, n);

        float vBias = m_settings.SI_bias / deltaTime *
                      glm::max(m_settings.floorHeight + c.radius - loc.y, 0.0f);
        float Pn =
            (vBias - m_settings.restitution * glm::min(vDotN, 0.0f)) / kn;
        glm::vec3 normalImpulse = Pn * n;

        glm::vec3 frictionImpulse(0.0f);
        glm::vec3 vPerp = vLoc - n * vDotN;
        float vPerpMag = glm::length(vPerp);
        if (vPerpMag > 0.001f) {
          glm::vec3 T = vPerp / vPerpMag;
          float Pt = glm::clamp(
              -vPerpMag / kn,
              -m_settings.frictionCoeff * Pn,
              m_settings.frictionCoeff * Pn);
          frictionImpulse = Pt * T;
        }

        computeImpulseVelocityAtLocation(
            {rbIdx},
            loc,
            normalImpulse + frictionImpulse,
            dVLin,
            dVAng);
#endif
        // applyImpulseAtLocation({rbIdx}, loc, normalImpulse + frictionImpulse);
      }

      state.linearVelocity += dVLin;
      state.angularVelocity += dVAng;
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
  initState.translation = initState.prevTranslation = translation;
  initState.rotation = initState.prevRotation = rotation;
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
  float pointMass = 0.1f;

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
  state.prevTranslation = state.translation;

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

void PhysicsSystem::computeImpulseVelocityAtLocation(
    RigidBodyHandle h,
    const glm::vec3& loc,
    const glm::vec3& impulse,
    glm::vec3& dVLin,
    glm::vec3& dVAng) {
  const RigidBody& rb = m_rigidBodies[h.idx];
  const RigidBodyState& state = m_rigidBodyStates[h.idx];

  glm::vec3 r = loc - state.translation;
  float rMagSq = glm::dot(r, r);
  dVLin += rb.invMass * r * glm::dot(impulse, r) / rMagSq;
  glm::quat qc = glm::conjugate(state.rotation);
  dVAng += state.rotation * (rb.invMoi * (qc * glm::cross(r, impulse)));
}

void PhysicsSystem::applyImpulseAtLocation(
    RigidBodyHandle h,
    const glm::vec3& loc,
    const glm::vec3& impulse) {

  RigidBodyState& state = m_rigidBodyStates[h.idx];
  computeImpulseVelocityAtLocation(
      h,
      loc,
      impulse,
      state.linearVelocity,
      state.angularVelocity);
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