#include <Althea/Application.h>
#include <Althea/Containers/StackVector.h>
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

    state.linearVelocity +=
        glm::vec3(0.0f, -m_settings.gravity, 0.0f) * deltaTime;
    /*float speed = glm::length(state.linearVelocity);
    if (speed > m_settings.maxSpeed) {
      state.linearVelocity *= m_settings.maxSpeed / speed;
    }*/
    state.prevTranslation = state.translation;
    state.translation += state.linearVelocity * deltaTime;

    state.prevRotation = state.rotation;
    // TODO: ???
    //state.angularVelocity +=
    //    deltaTime * rb.invMoi *
    //    (/*torque ext */ -glm::cross(
    //        state.angularVelocity,
    //        rb.moi * state.angularVelocity));

    float angularSpeed = glm::length(state.angularVelocity);
    if (angularSpeed > 0.0001f) {
      /*if (angularSpeed > m_settings.maxAngularSpeed) {
         state.angularVelocity *= m_settings.maxAngularSpeed / angularSpeed;
         angularSpeed = m_settings.maxAngularSpeed;
       }*/

      m_dbgDrawLines->addLine(
          state.translation,
          state.translation + state.angularVelocity,
          colorCyan);

      state.rotation +=
          0.5f * deltaTime *
        (glm::quat(0.0f, state.angularVelocity) * state.rotation);
      state.rotation = glm::normalize(state.rotation);
    }

    for (const BoundCapsule& boundCollider : rb.capsules) {
      Capsule& c = m_registeredCapsules[boundCollider.handle.colliderIdx];
      c.a = state.rotation * boundCollider.bindPose.a + state.translation;
      c.b = state.rotation * boundCollider.bindPose.b + state.translation;
    }
  }

  // floor collisions
  struct StaticCollision {
    glm::vec3 targetVel;

    glm::vec3 nStatic;
    glm::vec3 rStatic;
    glm::vec3 rRB;
    uint32_t rigidBodyIdx;
  };
  ALTHEA_STACK_VECTOR(staticCollisions, StaticCollision, 1000);

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

        glm::vec3 locs[2];
        uint32_t collisions = 0;
        float padding = 0.25f;
        if (c.a.y - c.radius < m_settings.floorHeight + padding) {
          vec3& loc = locs[collisions++];
          loc = c.a;
          loc.y -= c.radius;
        }
        if (c.b.y - c.radius < m_settings.floorHeight + padding) {
          vec3& loc = locs[collisions++];
          loc = c.b;
          loc.y -= c.radius;
        }
        if (!collisions) {
          continue;
        }

        for (int colIdx = 0; colIdx < collisions; ++colIdx) {
          glm::quat qc = glm::inverse(state.rotation);

          const glm::vec3& loc = locs[colIdx];
          float pen = glm::max(m_settings.floorHeight - loc.y, 0.0f);

          /*if (pen <= 0.0f)
            continue;*/

          StaticCollision& col = staticCollisions.emplace_back();
          col.rigidBodyIdx = rbIdx;
          col.nStatic = glm::vec3(0.0f, 1.0f, 0.0f);
          glm::vec3 r = loc - state.translation;
          col.rRB = qc * r;
          col.rStatic = glm::vec3(loc.x, m_settings.floorHeight, loc.z);

          // glm::vec3 v =
          //    state.linearVelocity + glm::cross(state.angularVelocity, r);
          //// TODO: parameterize
          // float restitution = 0.0f;
          // float friction = 0.0f;
          // col.targetVel = glm::vec3(
          //    (1.0f - friction) * v.x,
          //    glm::max(v.y, -restitution * v.y),
          //    (1.0f - friction) * v.z);

          glm::vec3 rxn = glm::cross(col.rRB, qc * col.nStatic);

          // effective mass at r
          float w = rb.invMass + glm::dot(rxn, rb.invMoi * rxn);
          // assert(w >= 0.00001f);

          glm::vec3 P = pen / w * col.nStatic;

          state.translation += rb.invMass * P;

          // linearized rotation update
          //glm::vec3 rxP = glm::cross(col.rRB, qc * P);
          glm::vec3 rxP = glm::cross(col.rRB, qc * P);
          state.rotation +=
              0.5f * glm::quat(0.0f, state.rotation * (rb.invMoi * rxP)) * state.rotation;

          // TODO: needed?
          state.rotation = glm::normalize(state.rotation);
        }
      }
    }
  }

  struct PrevVelocity {
    glm::vec3 linear;
    glm::vec3 angular;
  };
  ALTHEA_STACK_VECTOR(prevVelocities, PrevVelocity, m_rigidBodies.size());
  prevVelocities.resize(m_rigidBodies.size());

  // update velocity predictions
  if (m_settings.enableVelocityUpdate) {
    for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
      const RigidBody& rb = m_rigidBodies[rbIdx];
      RigidBodyState& state = m_rigidBodyStates[rbIdx];

      prevVelocities[rbIdx].linear = state.linearVelocity;
      prevVelocities[rbIdx].angular = state.angularVelocity;

      state.linearVelocity =
          (state.translation - state.prevTranslation) / deltaTime;

      glm::quat dQ = state.rotation * glm::inverse(state.prevRotation);
      state.angularVelocity = 2.0f / deltaTime * glm::vec3(dQ.x, dQ.y, dQ.z);
    /*  if (dQ.w < 0.0f)
        state.angularVelocity = -state.angularVelocity;*/

      // damping
      /*state.linearVelocity +=
          -state.linearVelocity *
          glm::min(m_settings.linearDamping * deltaTime, 1.0f);
      state.angularVelocity +=
          -state.angularVelocity *
          glm::min(m_settings.angularDamping * deltaTime, 1.0f);*/
    }
  }

  // solver collision velocities
  if (m_settings.enableVelocityUpdate) {
    for (int i = 0; i < 1; ++i) {
      for (const StaticCollision& col : staticCollisions) {
        const RigidBody& rb = m_rigidBodies[col.rigidBodyIdx];
        RigidBodyState& state = m_rigidBodyStates[col.rigidBodyIdx];
        const PrevVelocity& prevVel = prevVelocities[col.rigidBodyIdx];

        const glm::quat& q = state.rotation;
        glm::quat qc = glm::inverse(q);

        glm::vec3 rxn = glm::cross(col.rRB, qc * col.nStatic);

        // effective mass at r
        float w = rb.invMass + glm::dot(rxn, rb.invMoi * rxn);
        // assert(w > 0.0001f);

        glm::vec3 r = q * col.rRB;

        glm::vec3 v =
          state.linearVelocity + glm::cross(state.angularVelocity, r);
        glm::vec3 pv = prevVel.linear + glm::cross(prevVel.angular, r);
        float vn = glm::dot(v, col.nStatic);
        float pvn = glm::dot(pv, col.nStatic);
        glm::vec3 vt = v - vn * col.nStatic;
        float vtMag = glm::length(vt);



        // debug draw lines
        {
          glm::vec3 loc = state.translation + r;
          // from center of mass to collision point
          m_dbgDrawLines->addLine(loc, state.translation, colorOrange);
          // rigid body velocity at collision point
          m_dbgDrawLines->addLine(loc, loc + v * deltaTime, colorGreen);
        }

        if (vn >= 0.0f)
          continue;

        // restitution
        // glm::vec3 dV = col.nStatic*(-vn + glm::min(-m_settings.restitution *
        // pvn, 0.0f));
        // TODO
        glm::vec3 dV = -col.nStatic * vn;

        // friction
        float fn = 1.0f; // TODO
        /*glm::vec3 dV =
            -vtMag / vt* glm::min(
                     deltaTime * m_settings.frictionCoeff * glm::abs(fn),
                     vtMag);*/

                     // damping
                     /* dV += -state.linearVelocity *
                             glm::min(m_settings.linearDamping * deltaTime, 1.0f);
                      state.angularVelocity += -state.angularVelocity *
                          glm::min(m_settings.angularDamping * deltaTime, 1.0f);*/

        glm::vec3 p = dV / w;
        state.linearVelocity += p * rb.invMass;
        glm::vec3 rxp = glm::cross(col.rRB, qc * p);
        glm::vec3 dw = state.rotation * (rb.invMoi * rxp);
        state.angularVelocity += dw;

        /*if (glm::length(state.angularVelocity) > 10.0f)
          __debugbreak();*/

        // debug draw lines
        {
          glm::vec3 loc = state.translation + r;
          // rigid body impulse at collision point
          m_dbgDrawLines->addLine(loc, loc + dV * deltaTime, colorCyan);
        }
      }
    }
  }

  // TODO: velocity damping
  forceUpdateCapsules();

  // update visualization

  for (uint32_t i = 0; i < m_registeredCapsules.size(); ++i) {
    const Capsule& c = m_registeredCapsules[i];

    bool bFoundCollision = false;
    // for (uint32_t j = i + 1; j < m_registeredCapsules.size(); ++j) {
    //   CollisionResult result{};
    //   if (Collisions::checkIntersection(c, m_registeredCapsules[j], result))
    //   {
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
  float density = 1.0f;

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

      // TODO: Fix
      I[0][0] = I[1][1] = I[2][2] = m * r2;

      // rigid body space (tensor rotation)
      I = R * I * glm::transpose(R);

      // parallel axis theorem
      glm::vec3 D = C;
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

  rb.moi = moi;
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

void PhysicsSystem::forceUpdateCapsules() {
  for (uint32_t i = 0; i < m_rigidBodies.size(); ++i) {
    const RigidBody& rb = m_rigidBodies[i];
    RigidBodyState& state = m_rigidBodyStates[i];
    for (const BoundCapsule& boundCollider : rb.capsules) {
      Capsule& c = m_registeredCapsules[boundCollider.handle.colliderIdx];
      c.a = state.rotation * boundCollider.bindPose.a + state.translation;
      c.b = state.rotation * boundCollider.bindPose.b + state.translation;
    }
  }
}
} // namespace AltheaPhysics
} // namespace AltheaEngine