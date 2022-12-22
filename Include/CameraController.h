#pragma once

#include "Camera.h"
#include "InputManager.h"

#include <glm/glm.hpp>

namespace AltheaEngine {
class CameraController {
public:
  CameraController(InputManager& inputManager, float fovDegrees, float aspectRatio);
  ~CameraController();
  
  void tick(float deltaTime);

  const Camera& getCamera() const {
    return this->_camera;
  }

private:
  // Mouse updates, using normalized screen coordinates.
  void _updateMouse(float x, float y);

  void _updateTargetDirection(uint32_t axis, int dir);

  // The amount of time it takes for the actual velocity to converge
  // to the desired velocity.
  float _velocitySettleTime = 1.0f;
  glm::vec3 _velocity{};

  // Each axis will be [-1.0, 0.0, 1.0]. Normalize the vector to maintain
  // a consistent overall magnitude.
  glm::vec3 _targetDirection{};
  float _targetSpeed = 5.0f;

  Camera _camera;
};
} // namespace AltheaEngine