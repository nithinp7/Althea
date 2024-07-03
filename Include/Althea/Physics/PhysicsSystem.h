#pragma once

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

class PhysicsSystem {
public:
  PhysicsSystem() = default;

  PhysicsSystem(
      Application& app,
      SingleTimeCommandBuffer& commandBuffer,
      SceneToGBufferPassBuilder& gBufferPassBuilder,
      GlobalHeap& heap);

  void tick(float deltaTime);

  uint32_t
  registerCapsuleCollider(const glm::vec3& a, const glm::vec3& b, float radius);

  void updateCapsule(uint32_t idx, const glm::vec3& a, const glm::vec3& b);
  void updateCapsule(
      uint32_t idx,
      const glm::vec3& a,
      const glm::vec3& b,
      float radius);

  const Capsule& getCapsule(uint32_t idx) const {
    return m_registeredCapsules[idx];
  }

  uint32_t getCapsuleCount() const { return m_registeredCapsules.size(); }

private:
  std::vector<Capsule> m_registeredCapsules;

  IntrusivePtr<DebugDrawLines> m_dbgDrawLines;
  IntrusivePtr<DebugDrawCapsules> m_dbgDrawCapsules;
};
} // namespace AltheaPhysics
} // namespace AltheaEngine