#pragma once

#include "Camera.h"
#include "InputManager.h"

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
  // Camera movement callbacks:
  void _moveForward();
  void _moveBackward();
  void _strafeLeft();
  void _strafeRight();
  void _ascend();
  void _descend();

  // Mouse updates, using normalized screen coordinates.
  void _updateMouse(float x, float y);

  float _movementSpeed = 0.1f;

  Camera _camera;
};
} // namespace AltheaEngine