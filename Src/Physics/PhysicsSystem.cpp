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
  m_dbgDrawLines->reset();
  m_dbgDrawCapsules->reset();

  uint32_t colorRed = 0xff0000ff;
  for (const Capsule& c : m_registeredCapsules)
    m_dbgDrawCapsules->addCapsule(c.a, c.b, c.radius, colorRed);
}

uint32_t PhysicsSystem::registerCapsuleCollider(
    const glm::vec3& a,
    const glm::vec3& b,
    float radius) {
  uint32_t idx = m_registeredCapsules.size();
  m_registeredCapsules.push_back({a, b, radius});
  return idx;
}

void PhysicsSystem::updateCapsule(
    uint32_t idx,
    const glm::vec3& a,
    const glm::vec3& b) {
  m_registeredCapsules[idx].a = a;
  m_registeredCapsules[idx].b = b;
}

void PhysicsSystem::updateCapsule(
    uint32_t idx,
    const glm::vec3& a,
    const glm::vec3& b,
    float radius) {
  m_registeredCapsules[idx].a = a;
  m_registeredCapsules[idx].b = b;
  m_registeredCapsules[idx].radius = radius;
}
} // namespace AltheaPhysics
} // namespace AltheaEngine