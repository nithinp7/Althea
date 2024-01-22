#include "OrbitCamera.h"

#include "InputMask.h"

namespace AltheaEngine {
OrbitCamera::OrbitCamera(
    const glm::vec3& target,
    float spacing,
    float fovDegrees,
    float aspectRatio)
    : m_lookTarget(target),
      m_spacingTarget(spacing),
      m_camera(fovDegrees, aspectRatio, 0.01f, 1000.0f) {}

void OrbitCamera::tick(
    float deltaTime,
    uint32_t prevInputMask,
    uint32_t inputMask) {
  // clamp delta time
  deltaTime = glm::min(deltaTime, 1.0f / 30.0f);

  // Process key-board inputs
  if (inputMask & INPUT_BIT_A)
    m_targetYawRate = m_maxRotationRate;
  else if (inputMask & INPUT_BIT_D)
    m_targetYawRate = -m_maxRotationRate;
  else
    m_targetYawRate = 0.0f;

  if (inputMask & INPUT_BIT_W)
    m_targetPitchRate = m_maxRotationRate;
  else if (inputMask & INPUT_BIT_S)
    m_targetPitchRate = -m_maxRotationRate;
  else
    m_targetPitchRate = 0.0f;

  if (inputMask & INPUT_BIT_Q)
    m_spacingTarget += 0.1f;
  else if (inputMask & INPUT_BIT_E)
    m_spacingTarget -= 0.1f;

  m_spacingTarget = glm::clamp(m_spacingTarget, 5.0f, 200.0f);

  float tSettle = 1.0f;
  float K = 4.0f / tSettle;
  float yawAccel = K * (m_targetYawRate - m_yawRate);
  float pitchAccel = K * (m_targetPitchRate - m_pitchRate);

  m_yawRate += yawAccel * deltaTime;
  m_pitchRate += pitchAccel * deltaTime;

  m_yaw += m_yawRate * deltaTime;
  m_pitch += m_pitchRate * deltaTime;

  m_camera.setRotationRadians(-m_yaw, -m_pitch);
  glm::vec3 cameraFront(m_camera.getTransform()[2]);
  glm::vec3 cameraPosition = m_lookTarget + m_spacingTarget * cameraFront;
  m_camera.setPosition(cameraPosition);
  // TODO: Clamp pitch
}
} // namespace AltheaEngine