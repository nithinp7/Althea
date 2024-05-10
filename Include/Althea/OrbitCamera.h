#pragma once

#include "Camera.h"
#include "Library.h"

#include <glm/glm.hpp>

#include <cstdint>

namespace AltheaEngine {
class ALTHEA_API OrbitCamera {
public:
  OrbitCamera() = default;
  OrbitCamera(
      const glm::vec3& target,
      float spacing,
      float fovDegrees,
      float aspectRatio);
  void tick(float deltaTime, uint32_t prevInputMask, uint32_t inputMask);

  const Camera& getCamera() const { return m_camera; }
  Camera& getCamera() { return m_camera; }

  void setRotation(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = pitch;
  }
  float getSpacing() const { return m_spacingTarget; }
  void setSpacing(float spacing) { m_spacingTarget = spacing; }
  void setTargetPosition(const glm::vec3& target) { m_lookTarget = target; }

private:
  Camera m_camera;

  glm::vec3 m_lookTarget;
  float m_spacingTarget;

  // radians
  float m_yaw = 0.0f;
  float m_pitch = 0.0f;

  // radians per second
  float m_yawRate = 0.0f;
  float m_pitchRate = 0.0f;
  float m_targetYawRate = 0.0f;
  float m_targetPitchRate = 0.0f;
  float m_maxRotationRate = 2.0f;
};
} // namespace AltheaEngine