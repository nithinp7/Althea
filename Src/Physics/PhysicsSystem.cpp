#include <Althea/Application.h>
#include <Althea/Containers/StackVector.h>
#include <Althea/Physics/PhysicsSystem.h>
#include <Althea/Utilities.h>
#include <glm/gtx/quaternion.hpp>

#define COLOR_RED 0xff0000ff
#define COLOR_ORANGE 0xdd8822ff
#define COLOR_GREEN 0x00ff00ff
#define COLOR_BLUE 0x0000ffff
#define COLOR_CYAN 0x00aaffff

namespace AltheaEngine {
namespace AltheaPhysics {

PhysicsSystem::PhysicsSystem(
    Application& app,
    SingleTimeCommandBuffer& commandBuffer,
    SceneToGBufferPassBuilder& gBufferPassBuilder,
    GlobalHeap& heap)
    : m_dbgDrawLines(makeIntrusive<DebugDrawLines>(app)),
      m_dbgDrawCapsules(
          makeIntrusive<DebugDrawCapsules>(app, commandBuffer, 1000, false)),
      m_dbgDrawCapsulesWireframe(
          makeIntrusive<DebugDrawCapsules>(app, commandBuffer, 1000, true)) {
  gBufferPassBuilder.registerSubpass(m_dbgDrawLines);
  gBufferPassBuilder.registerSubpass(m_dbgDrawCapsules);
  gBufferPassBuilder.registerSubpass(m_dbgDrawCapsulesWireframe);
}

void PhysicsSystem::tick(float deltaTime) {
  m_dbgDrawLines->reset();
  m_dbgDrawCapsules->reset();
  m_dbgDrawCapsulesWireframe->reset();

  float h = deltaTime / m_settings.timeSubsteps;

  xpbd_findCollisions();

  for (uint32_t substepIter = 0; substepIter < m_settings.timeSubsteps;
       ++substepIter) {
    xpbd_integrateState(h);

    for (uint32_t posIter = 0; posIter < m_settings.positionIterations;
         ++posIter) {
      xpbd_solveCollisionPositions();
    }

    if (m_settings.enableVelocityUpdate) {
      xpbd_predictVelocities(h);
      xpbd_solveCollisionVelocities(h);
    }
  }

  forceUpdateCapsules();

  forceDebugDraw(deltaTime);
}

void PhysicsSystem::xpbd_integrateState(float h) {
  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

    // state.prevLinearVelocity = state.linearVelocity;
    state.linearVelocity += glm::vec3(0.0f, -m_settings.gravity, 0.0f) * h;
    /*float speed = glm::length(state.linearVelocity);
    if (speed > m_settings.maxSpeed) {
      state.linearVelocity *= m_settings.maxSpeed / speed;
    }*/
    state.prevTranslation = state.translation;
    state.translation += state.linearVelocity * h;

    state.prevRotation = state.rotation;

    glm::mat3 R(state.rotation);
    glm::mat3 Rt = glm::transpose(R);

    // world space moment of inertia
    //glm::mat3 I = R * rb.moi * Rt; // TODO: ??
    glm::mat3 I = Rt * rb.moi * R; // TODO: ??
    glm::mat3 I_inv = Rt * rb.invMoi * R;
    //glm::mat3 I_inv = R * rb.invMoi * Rt;
    
    glm::quat qc = glm::inverse(state.rotation);

    state.angularVelocity +=
        (h * I_inv *
         (/*torque ext */ -glm::cross(
             state.angularVelocity,
             I * state.angularVelocity)));

    // float angularSpeed = glm::length(state.angularVelocity);
    // if (angularSpeed > 0.0001f)
    {
      /*if (angularSpeed > m_settings.maxAngularSpeed) {
         state.angularVelocity *= m_settings.maxAngularSpeed / angularSpeed;
         angularSpeed = m_settings.maxAngularSpeed;
       }*/

      state.rotation +=
          0.5f * h * (glm::quat(0.0f, state.angularVelocity) * state.rotation);
      state.rotation = glm::normalize(state.rotation);
    }
  }

  forceUpdateCapsules();
}

void PhysicsSystem::xpbd_findCollisions() {
  m_staticCollisions.clear();
  m_dynamicCollisions.clear();

  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

    for (const BoundCapsule& boundCollider : rb.capsules) {
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

        StaticCollision& col = m_staticCollisions.emplace_back();

        const glm::vec3& loc = locs[colIdx];
        glm::vec3 r = loc - state.translation;

        col.rigidBodyIdx = rbIdx;
        col.nStatic = glm::vec3(0.0f, 1.0f, 0.0f);
        col.rRB = qc * r;
        col.rStatic = glm::vec3(loc.x, m_settings.floorHeight, loc.z);
        col.lambdaN = 0.0f;
        col.lambdaT = 0.0f;
      }
    }
  }

  // TODO: Spatial acceleration structures...
  for (uint32_t ia = 0; ia < m_registeredCapsules.size(); ++ia) {
    uint32_t rigidBodyAIdx = m_capsuleOwners[ia];
    if (rigidBodyAIdx == ~0)
      continue;

    for (uint32_t ib = ia + 1; ib < m_registeredCapsules.size(); ++ib) {
      uint32_t rigidBodyBIdx = m_capsuleOwners[ib];
      if (rigidBodyBIdx == ~0 || rigidBodyAIdx == rigidBodyBIdx)
        continue;

      const Capsule& a = m_registeredCapsules[ia];
      const Capsule& b = m_registeredCapsules[ib];

      CollisionResult result;
      if (Collisions::checkIntersection(a, b, result)) {
        const RigidBodyState& stateA = m_rigidBodyStates[rigidBodyAIdx];
        const RigidBodyState& stateB = m_rigidBodyStates[rigidBodyBIdx];

        glm::quat qac = glm::inverse(stateA.rotation);
        glm::quat qbc = glm::inverse(stateB.rotation);

        DynamicCollision& col = m_dynamicCollisions.emplace_back();
        col.rbAIdx = rigidBodyAIdx;
        col.rbBIdx = rigidBodyBIdx;
        col.rA = qac * (result.ra - stateA.translation);
        col.rB = qbc * (result.rb - stateB.translation);
        col.n = -result.n;
        col.lambdaN = 0.0f;
        col.lambdaT = 0.0f;
      }
    }
  }
}

void PhysicsSystem::xpbd_solveCollisionPositions() {
  for (StaticCollision& col : m_staticCollisions) {
    const RigidBody& rb = m_rigidBodies[col.rigidBodyIdx];
    RigidBodyState& state = m_rigidBodyStates[col.rigidBodyIdx];

    glm::quat qc = glm::inverse(state.rotation);

    glm::vec3 rxn = glm::cross(col.rRB, qc * col.nStatic);

    // effective mass at r
    float w = rb.invMass + glm::dot(rxn, rb.invMoi * rxn);

    glm::vec3 loc = state.translation + state.rotation * col.rRB;
    float C = glm::dot(col.rStatic - loc, col.nStatic);
    if (C <= 0.0f)
      continue;

    float dLambdaN = C / w;
    glm::vec3 P = dLambdaN * col.nStatic;
    col.lambdaN += dLambdaN;

    glm::vec3 prevLoc = state.prevTranslation + state.prevRotation * col.rRB;
    glm::vec3 locV = loc - prevLoc;
    glm::vec3 Pt = locV - glm::dot(locV, col.nStatic) * col.nStatic;
    float dLambdaT = glm::length(Pt);
    float mu_s = 0.0f; // m_settings.staticFriction;
    /*if (dLambdaT < mu_s * dLambdaN)
      P -= Pt;*/
    col.lambdaT += dLambdaT;

    state.translation += rb.invMass * P;

    // linearized rotation update
    glm::vec3 rxP = glm::cross(col.rRB, qc * P);
    state.rotation += 0.5f *
                      glm::quat(0.0f, state.rotation * (rb.invMoi * rxP)) *
                      state.rotation;
    state.rotation = glm::normalize(state.rotation);
  }

  if (m_settings.enableDynamicCollisions) {
    for (DynamicCollision& col : m_dynamicCollisions) {
      const RigidBody& rbA = m_rigidBodies[col.rbAIdx];
      RigidBodyState& stateA = m_rigidBodyStates[col.rbAIdx];
      const RigidBody& rbB = m_rigidBodies[col.rbBIdx];
      RigidBodyState& stateB = m_rigidBodyStates[col.rbBIdx];

      glm::vec3 a = stateA.translation + stateA.rotation * col.rA;
      glm::vec3 b = stateB.translation + stateB.rotation * col.rB;
      float C = glm::dot(b - a, col.n);
      if (C < 0.0f)
        continue;

      glm::quat qac = glm::inverse(stateA.rotation);
      glm::quat qbc = glm::inverse(stateB.rotation);

      glm::vec3 na = qac * col.n;
      glm::vec3 nb = qbc * col.n;

      glm::vec3 rxn_a = glm::cross(col.rA, na);
      glm::vec3 rxn_b = glm::cross(col.rB, nb);

      // effective mass at r
      float wa = rbA.invMass + glm::dot(rxn_a, rbA.invMoi * rxn_a);
      float wb = rbB.invMass + glm::dot(rxn_b, rbB.invMoi * rxn_b);

      float dLambdaN = C / (wa + wb);
      glm::vec3 P = dLambdaN * col.n;
      col.lambdaN += dLambdaN;

      // TODO: lambdaT + friction...

      // TODO: may need to flip signs...
      stateA.translation += P * rbA.invMass;
      stateB.translation -= P * rbB.invMass;

      // linearized rotation update
      glm::vec3 rxP_a = glm::cross(col.rA, qac * P);
      glm::vec3 rxP_b = glm::cross(col.rB, qbc * P);

      stateA.rotation +=
          0.5f * glm::quat(0.0f, stateA.rotation * (rbA.invMoi * rxP_a)) *
          stateA.rotation;
      stateA.rotation = glm::normalize(stateA.rotation);

      stateB.rotation -=
          0.5f * glm::quat(0.0f, stateB.rotation * (rbB.invMoi * rxP_b)) *
          stateB.rotation;
      stateB.rotation = glm::normalize(stateB.rotation);
    }
  }
}

void PhysicsSystem::xpbd_predictVelocities(float h) {
  for (uint32_t rbIdx = 0; rbIdx < m_rigidBodies.size(); ++rbIdx) {
    const RigidBody& rb = m_rigidBodies[rbIdx];
    RigidBodyState& state = m_rigidBodyStates[rbIdx];

    state.prevLinearVelocity = state.linearVelocity;
    state.linearVelocity = (state.translation - state.prevTranslation) / h;

    state.prevAngularVelocity = state.angularVelocity;
    glm::quat dQ = state.rotation * glm::inverse(state.prevRotation);
    state.angularVelocity = 2.0f / h * glm::vec3(dQ.x, dQ.y, dQ.z);
    if (dQ.w < 0.0f)
      state.angularVelocity = -state.angularVelocity;

    // damping
    /*state.linearVelocity +=
        -state.linearVelocity *
        glm::min(m_settings.linearDamping * deltaTime, 1.0f);
    state.angularVelocity +=
        -state.angularVelocity *
        glm::min(m_settings.angularDamping * deltaTime, 1.0f);*/
  }
}

void PhysicsSystem::xpbd_solveCollisionVelocities(float h) {
  for (const StaticCollision& col : m_staticCollisions) {
    const RigidBody& rb = m_rigidBodies[col.rigidBodyIdx];
    RigidBodyState& state = m_rigidBodyStates[col.rigidBodyIdx];

    const glm::quat& q = state.rotation;
    glm::quat qc = glm::inverse(q);

    glm::vec3 rxn = glm::cross(col.rRB, qc * col.nStatic);

    // effective mass at r
    float w = rb.invMass + glm::dot(rxn, rb.invMoi * rxn);
    // assert(w > 0.0001f);

    glm::vec3 r = q * col.rRB;

    glm::vec3 v = state.linearVelocity + glm::cross(state.angularVelocity, r);
    glm::vec3 pv =
        state.prevLinearVelocity + glm::cross(state.prevAngularVelocity, r);
    float vn = glm::dot(v, col.nStatic);
    float pvn = glm::dot(pv, col.nStatic);
    glm::vec3 vt = v - vn * col.nStatic;
    float vtMag = glm::length(vt);

    /*if (vn >= 0.0f)
      continue;*/

    // restitution
    // glm::vec3 dV = col.nStatic*(-vn + glm::min(-m_settings.restitution *
    // pvn, 0.0f));
    // TODO
    glm::vec3 dV = -col.nStatic * vn;

    // friction
    float fn_h = col.lambdaN / h;
    float mu_d = m_settings.dynamicFriction;
    if (vtMag > 0.0001f) {
      glm::vec3 dV_friction = -vt / vtMag * glm::min(fn_h * mu_d, vtMag);
      if (glm::length(dV_friction) > 0.01f)
        dV += dV_friction;
      {
        glm::vec3 loc = state.translation + r;
        m_dbgDrawLines->addLine(loc, loc + dV_friction, COLOR_RED);
      }
    }

    // damping
    /*dV += -state.linearVelocity *
            glm::min(m_settings.linearDamping * deltaTime, 1.0f);
     state.angularVelocity += -state.angularVelocity *
         glm::min(m_settings.angularDamping * deltaTime, 1.0f);*/

    glm::vec3 p = dV / w;
    state.linearVelocity += p * rb.invMass;
    glm::vec3 rxp = glm::cross(col.rRB, qc * p);
    state.angularVelocity += state.rotation * (rb.invMoi * rxp);
  }

  for (const DynamicCollision& col : m_dynamicCollisions) {
    const RigidBody& rba = m_rigidBodies[col.rbAIdx];
    const RigidBody& rbb = m_rigidBodies[col.rbBIdx];

    RigidBodyState& stateA = m_rigidBodyStates[col.rbAIdx];
    RigidBodyState& stateB = m_rigidBodyStates[col.rbBIdx];

    const glm::quat& qa = stateA.rotation;
    const glm::quat& qb = stateB.rotation;

    glm::quat qac = glm::inverse(qa);
    glm::quat qbc = glm::inverse(qb);

    glm::vec3 rxn_a = glm::cross(col.rA, qac * col.n);
    glm::vec3 rxn_b = glm::cross(col.rB, qbc * col.n);

    // effective mass at r
    float wa = rba.invMass + glm::dot(rxn_a, rba.invMoi * rxn_a);
    float wb = rbb.invMass + glm::dot(rxn_b, rbb.invMoi * rxn_b);

    glm::vec3 ra = qa * col.rA;
    glm::vec3 rb = qb * col.rB;

    glm::vec3 va =
        stateA.linearVelocity + glm::cross(stateA.angularVelocity, ra);
    glm::vec3 vb =
        stateB.linearVelocity + glm::cross(stateB.angularVelocity, rb);

    glm::vec3 pva =
        stateA.prevLinearVelocity + glm::cross(stateA.prevAngularVelocity, ra);
    glm::vec3 pvb =
        stateB.prevLinearVelocity + glm::cross(stateB.prevAngularVelocity, rb);

    glm::vec3 v = vb - va;
    glm::vec3 pv = pvb - pva;

    float vn = glm::dot(v, col.n);
    float pvn = glm::dot(pv, col.n);

    glm::vec3 vt = v - vn * col.n;
    float vtMag = glm::length(vt);

    glm::vec3 locA = stateA.translation + ra;
    glm::vec3 locB = stateB.translation + rb;

    /*if (vn >= 0.0f)
      continue;*/

    // restitution
    // glm::vec3 dV = col.nStatic*(-vn + glm::min(-m_settings.restitution *
    // pvn, 0.0f));

    // TODO negative here??
    glm::vec3 dV = col.n * vn;

    float fn_h = abs(col.lambdaN / h);
    float mu_d = m_settings.dynamicFriction;
    if (vtMag > 0.0001f) {
      glm::vec3 dV_friction = vt / vtMag * glm::min(fn_h * mu_d, vtMag);
      if (glm::length(dV_friction) > 0.01f)
        dV += dV_friction;
      { m_dbgDrawLines->addLine(locA, locA + dV_friction, COLOR_RED); }
    }
    // friction
    // float lambdaN = C / (wa + wb);
    // float fn = abs(lambdaN); // TODO
    // dV += -vt / vtMag * glm::min(h * m_settings.dynamicFriction * fn, vtMag);

    // damping
    /* dV += -state.linearVelocity *
            glm::min(m_settings.linearDamping * deltaTime, 1.0f);
     state.angularVelocity += -state.angularVelocity *
         glm::min(m_settings.angularDamping * deltaTime, 1.0f);*/

    glm::vec3 p = dV / (wa + wb);
    stateA.linearVelocity += p * rba.invMass;
    stateB.linearVelocity -= p * rbb.invMass;

    glm::vec3 rxp_a = glm::cross(col.rA, qac * p);
    glm::vec3 rxp_b = glm::cross(col.rB, qbc * p);

    stateA.angularVelocity += stateA.rotation * (rba.invMoi * rxp_a);
    stateB.angularVelocity -= stateB.rotation * (rbb.invMoi * rxp_b);
  }
}

void PhysicsSystem::forceDebugDraw(float deltaTime) {
  m_dbgDrawLines->reset();
  m_dbgDrawCapsules->reset();
  m_dbgDrawCapsulesWireframe->reset();

  debugDraw(deltaTime);
}

typedef std::vector<bool> Bitset;

void PhysicsSystem::debugDraw(float deltaTime) {
  if (m_settings.debugDrawCapsules) {
    Utilities::pushRandSeed(123);

    static Bitset s_collidingMask;
    s_collidingMask.clear();
    s_collidingMask.resize(m_rigidBodies.size());

    for (const DynamicCollision& col : m_dynamicCollisions) {
      s_collidingMask[col.rbAIdx] = true;
      s_collidingMask[col.rbBIdx] = true;
    }
    /*for (const StaticCollision& col : m_staticCollisions) {
      s_collidingMask[col.rigidBodyIdx] = true;
    }*/

    for (uint32_t i = 0; i < m_rigidBodies.size(); ++i) {
      const RigidBody& rb = m_rigidBodies[i];
      const RigidBodyState& state = m_rigidBodyStates[i];

      glm::mat4 model = glm::toMat4(state.rotation);
      model[3] = glm::vec4(state.translation, 1.0f);

      uint32_t color =
          (static_cast<uint32_t>((float)0xffffff * Utilities::randf()) << 8) |
          0xff;

      bool bIsColliding = s_collidingMask[i];

      for (const BoundCapsule& c : rb.capsules) {
        if (m_settings.wireframeCapsules) {
          m_dbgDrawCapsulesWireframe->addCapsule(
              model,
              c.bindPose.a,
              c.bindPose.b,
              c.bindPose.radius,
              bIsColliding ? COLOR_GREEN : COLOR_BLUE);
        } else {
          m_dbgDrawCapsules->addCapsule(
              model,
              c.bindPose.a,
              c.bindPose.b,
              c.bindPose.radius,
              color);
        }
      }
    }

    uint32_t lastSeedState = Utilities::getRandSeed();

    Utilities::popRandSeed();

    // unbound capsules
    for (uint32_t i = 0; i < m_registeredCapsules.size(); ++i) {
      if (m_capsuleOwners[i] == ~0) {
        const Capsule& c = m_registeredCapsules[i];

        bool bIsColliding = false;
        for (uint32_t j = 0; j < m_registeredCapsules.size(); ++j) {
          if (i != j && m_capsuleOwners[j] == ~0) {
            const Capsule& c2 = m_registeredCapsules[j];

            CollisionResult result;
            if (Collisions::checkIntersection(c, c2, result)) {
              bIsColliding = true;
              m_dbgDrawLines->addLine(result.ra, result.rb, COLOR_RED);
            }
          }
        }

        if (m_settings.wireframeCapsules)
          m_dbgDrawCapsulesWireframe->addCapsule(
              c.a,
              c.b,
              c.radius,
              bIsColliding ? COLOR_GREEN : COLOR_BLUE);
        else {
          uint32_t color =
              (static_cast<uint32_t>(
                   (float)0xffffff * Utilities::randf(lastSeedState))
               << 8) |
              0xff;
          m_dbgDrawCapsules->addCapsule(
              c.a,
              c.b,
              c.radius,
              bIsColliding ? COLOR_GREEN : color);
        }
      }
    }
  }

  if (m_settings.debugDrawVelocities) {
    for (uint32_t i = 0; i < m_rigidBodies.size(); ++i) {
      const RigidBodyState& state = m_rigidBodyStates[i];

      m_dbgDrawLines->addLine(
          state.translation,
          state.translation + deltaTime * state.linearVelocity,
          COLOR_GREEN);
      m_dbgDrawLines->addLine(
          state.translation,
          state.translation + deltaTime * state.angularVelocity,
          COLOR_CYAN);
    }
  }

  if (m_settings.debugDrawCollisions) {
    for (const StaticCollision& col : m_staticCollisions) {
      const RigidBody& rb = m_rigidBodies[col.rigidBodyIdx];
      const RigidBodyState& state = m_rigidBodyStates[col.rigidBodyIdx];

      glm::vec3 loc = state.translation + state.rotation * col.rRB;
      // from center of mass to collision point
      m_dbgDrawLines->addLine(loc, state.translation, COLOR_ORANGE);
      // rigid body velocity at collision point
      m_dbgDrawLines->addLine(
          loc,
          loc + state.linearVelocity * deltaTime,
          COLOR_GREEN);
    }

    for (const DynamicCollision& col : m_dynamicCollisions) {
      const RigidBodyState& stateA = m_rigidBodyStates[col.rbAIdx];
      const RigidBodyState& stateB = m_rigidBodyStates[col.rbBIdx];

      glm::vec3 a = stateA.translation + stateA.rotation * col.rA;
      glm::vec3 b = stateB.translation + stateB.rotation * col.rB;
      m_dbgDrawLines->addLine(
          a,
          b,
          glm::dot(b - a, col.n) < 0.0f ? COLOR_BLUE : COLOR_RED);
    }
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
  m_capsuleOwners.push_back(~0);
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
  m_capsuleOwners[c.colliderIdx] = rb.idx;
}

void PhysicsSystem::bakeRigidBody(RigidBodyHandle h) {
  RigidBody& rb = m_rigidBodies[h.idx];
  RigidBodyState& state = m_rigidBodyStates[h.idx];

  float mass = 0.0f;
  glm::mat3 moi(0.0f);
  glm::vec3 com(0.0f);

  // TODO: paramaterize
  float density = 10.0f;

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
      // I = R * I * glm::transpose(R);
      // I = glm::transpose(R) * I * R;

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

      // rigid body space (tensor rotation)
      I = R * I * glm::transpose(R);
      // I = glm::transpose(R) * I * R;
    }

    // superposition
    moi += I;
  }

  rb.moi = moi;
  rb.invMoi = glm::inverse(moi);
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