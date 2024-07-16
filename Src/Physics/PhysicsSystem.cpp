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
  m_dbgDrawLines->reset();
  m_dbgDrawCapsules->reset();

  uint32_t colorRed = 0xff0000ff;
  uint32_t colorOrange = 0xdd8822ff;
  uint32_t colorGreen = 0x00ff00ff;
  uint32_t colorBlue = 0x0000ffff;
  uint32_t colorCyan = 0x00aaffff;

  // update rigid bodies

  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

#ifdef XPBD
    state.linearVelocity =
        (state.translation - state.prevTranslation) / deltaTime;
    state.prevTranslation = state.translation;

    glm::quat dQ = state.rotation *
                   glm::inverse(state.prevRotation); //(state.prevRotation);
    state.angularVelocity = 2.0f / deltaTime * glm::vec3(dQ.x, dQ.y, dQ.z);
    if (dQ.w < 0.0f)
      state.angularVelocity = -state.angularVelocity;
    // ???
    state.angularVelocity +=
        deltaTime * rb.invMoi *
        (/*torque ext */ -glm::cross(
            state.angularVelocity,
            glm::inverse(rb.invMoi) * state.angularVelocity));
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

    // TODO :apply angular torque
    float angularSpeed = glm::length(state.angularVelocity);
    if (angularSpeed > 0.001f) {
      if (angularSpeed > m_settings.maxAngularSpeed) {
        state.angularVelocity *= m_settings.maxAngularSpeed / angularSpeed;
        angularSpeed = m_settings.maxAngularSpeed;
      }

      // glm::quat dQ = glm::angleAxis(
      //     angularSpeed * deltaTime,
      //     state.angularVelocity / angularSpeed);
      // state.rotation = glm::normalize(state.rotation);
      // state.rotation = dQ * glm::normalize(state.rotation);
      m_dbgDrawLines->addLine(
          state.translation,
          state.translation + state.angularVelocity,
          colorCyan);
      state.rotation += deltaTime * 0.5f *
                        glm::quat(0.0f, state.angularVelocity) * state.rotation;
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

      for (const BoundCapsule& boundCollider : rb.capsules) {
        // TODO: Move this to Collisions
        // TODO: Need a better way to update position of colliders after
        // constraint solver steps... const Capsule& c =
        //     m_registeredCapsules[boundCollider.handle.colliderIdx];
        Capsule c = boundCollider.bindPose;
        c.a = state.rotation * c.a + state.translation;
        c.b = state.rotation * c.b + state.translation;

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
        // if (pen < 0.0001f)
        //   continue;

        glm::vec3 n(0.0f, 1.0f, 0.0f);
        glm::vec3 r = loc - state.translation;
        m_dbgDrawLines->addLine(loc, state.translation, colorOrange);

        glm::quat qc = glm::inverse(state.rotation);

        glm::vec3 rxn = glm::cross(qc * r, qc * n);

        // effective mass at r
        float w = rb.invMass + glm::dot(rxn, rb.invMoi * rxn);
        // w = abs(w);
        assert(w >= 0.001f);

        float stiffness = 1.0f; // 0.5f; // * deltaTime;
        glm::vec3 P = stiffness * pen / w * n;

        float rMagSq = glm::dot(r, r);
        // state.translation += rb.invMass * r * glm::dot(P, r) / rMagSq;
        state.translation += rb.invMass * P;
        // /*TODO: REMOVE*/ state.prevTranslation = state.translation;

        // linearized rotation update
        glm::vec3 rxP = glm::cross(qc * r, qc * P);
        m_dbgDrawLines->addLine(loc, loc + state.rotation * rxP, colorGreen);
        m_dbgDrawLines->addLine(loc, loc + P, colorCyan);

        // state.rotation += 0.5f *
        //                   glm::quat(0.0f, 0.01f * glm::cross(r, P) *
        //                   rb.invMass) * // state.rotation * (rb.invMoi *
        //                   rxP)) * state.rotation;
        state.rotation +=
            0.5f * glm::quat(0.0f, rb.invMoi * rxP) * state.rotation;
        // 0.5f * glm::quat(0.0f, rb.invMoi * glm::cross(r, P)) *
        // state.rotation;
        state.rotation = glm::normalize(state.rotation);
        // /*TODO: REMOVE*/ state.prevRotation = state.rotation;
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
    }
  }

  for (uint32_t i = 0; i < m_rigidBodies.size(); ++i) {
    const RigidBody& rb = m_rigidBodies[i];
    RigidBodyState& state = m_rigidBodyStates[i];
    for (const BoundCapsule& boundCollider : rb.capsules) {
      Capsule& c = m_registeredCapsules[boundCollider.handle.colliderIdx];
      c.a = state.rotation * boundCollider.bindPose.a + state.translation;
      c.b = state.rotation * boundCollider.bindPose.b + state.translation;
    }
  }

  // update visualization

  for (uint32_t i = 0; i < m_registeredCapsules.size(); ++i) {
    const Capsule& c = m_registeredCapsules[i];

    bool bFoundCollision = false;
    // for (uint32_t j = i + 1; j < m_registeredCapsules.size(); ++j) {
    //   CollisionResult result{};
    //   if (Collisions::checkIntersection(c, m_registeredCapsules[j], result)) {
    //     m_dbgDrawLines->addLine(
    //         result.intersectionPoint - 0.5f * result.minSepTranslation,
    //         result.intersectionPoint + 0.5f * result.minSepTranslation,
    //         colorGreen);
    //     bFoundCollision = true;
    //     break;
    //   }
    // }

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

  float mass = 0.0f;
  glm::mat3 moi(0.0f);
  glm::vec3 com(0.0f);

  // TODO: paramaterize
  float density = 0.1f;

  auto compCapsuleMass = [&](const Capsule& c) {
    return density * 2.0f * glm::pi<float>() * c.radius *
           glm::length(c.a - c.b);
  };

  for (const BoundCapsule& c : rb.capsules) {
    glm::vec3 center = 0.5f * (c.bindPose.a + c.bindPose.b);
    float m = compCapsuleMass(c.bindPose);
    com += center * m;
    mass += m;
  }

  if (mass > 0.0f) {
    rb.invMass = 1.0f / mass;
    com *= rb.invMass;
  }

  // rebase capsules to new center of mass
  for (BoundCapsule& c : rb.capsules) {
    c.bindPose.a -= com;
    c.bindPose.b -= com;
  }

  state.translation += com;
  state.prevTranslation = state.translation;

  // recompute moment of inertia
  // (treats capsules as cylinders...)
  for (const BoundCapsule& capsule : rb.capsules) {
    float m = compCapsuleMass(capsule.bindPose);
    glm::vec3 C = 0.5f * (capsule.bindPose.a + capsule.bindPose.b);
    glm::vec3 L = capsule.bindPose.b - capsule.bindPose.a;
    float l2 = glm::dot(L, L);
    float l = sqrt(l2);
    float r = capsule.bindPose.radius;
    float r2 = r * r;

    // local capsule to rigid body space rotation
    glm::mat3 R;
    {
      R[0] = L / l;
      R[1] = glm::cross(glm::vec3(0.0, 1.0, 0.0), R[0]);
      float R1Mag = glm::length(R[1]);
      if (R1Mag > 0.001f)
        R[1] /= R1Mag;
      else
        R[1] = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), R[0]));
      R[2] = glm::cross(R[0], R[1]);
    }

    // partial moment of inertia
    glm::mat3 I(0.0f);
    {
      // local moment of inertia
      I[0][0] = 0.5f * m * r2;
      I[1][1] = I[2][2] = m * (l2 + 3.0f * r2) / 12.0f;

      // rigid body space (tensor rotation)
      I = R * I * glm::transpose(R);

      // parallel axis theorem
      glm::vec3 D = -C;
      float DMag2 = glm::dot(D, D);
      for (int i = 0; i < 3; ++i) {
        I[i][i] += m * (DMag2 - D[i] * D[i]);
        for (int j = 0; j < 3; ++j) {
          if (i == j)
            continue;
          float f = m * (-D[i] * D[j]);
          I[i][j] += f;
          I[j][i] += f;
        }
      }
    }

    // superposition
    moi += I;
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