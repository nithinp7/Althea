#pragma once

#include "Camera.h"
#include "InputManager.h"

#include <glm/glm.hpp>

namespace AltheaEngine {
class CameraController {
public:
  CameraController(
      InputManager& inputManager,
      float fovDegrees,
      float aspectRatio);
  ~CameraController();

  void tick(float deltaTime);

  const Camera& getCamera() const { return this->_camera; }

  Camera& getCamera() { return this->_camera; }

private:
  // Mouse updates, using normalized screen coordinates.
  void _updateMouse(double x, double y, bool cursorHidden);

  void _updateTargetDirection(uint32_t axis, int dir);

  // The amount of time it takes for the actual velocity to converge
  // to the desired velocity.
  float _velocitySettleTime = 1.0f;
  glm::vec3 _velocity{};

  // Each axis will be [-1.0, 0.0, 1.0]. Normalize the vector to maintain
  // a consistent overall magnitude.
  glm::vec3 _targetDirection{};
  float _targetSpeed = 5.0f;

  // We manually track the camera yaw so we can track winding.
  // This lets us unambiguously know the direction we should rotate the
  // camera during large mouse movements (larger than 180 degrees before the
  // feedback controller can catch up).
  float _yaw = 0.0f;

  // The amount of time it takes for the orientation to converge to
  // the desired orientation.
  float _orientationSettleTime = 0.5f;
  float _pitchRate = 0.0f;
  float _yawRate = 0.0f;
  float _targetPitch = 0.0f;
  float _targetYaw = 0.0f;

  float _yawMultiplier = 0.5f;   // 0.25f;
  float _pitchMultiplier = 1.0f; // 0.25f;

  Camera _camera;
};
} // namespace AltheaEngine