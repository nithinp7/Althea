#pragma once

#include <glm/glm.hpp>

namespace AltheaEngine {
class Camera {
public:
  Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);
  void setPosition(const glm::vec3& position);
  void setRotation(float yawDegrees, float pitchDegrees);

  const glm::mat4& getTransform() const {
    return this->_transform;
  }

  float computeYawDegrees() const;
  float computePitchDegrees() const;
  glm::mat4 computeView() const;

  const glm::mat4& getProjection() const {
    return this->_projection;
  }

private:
  glm::mat4 _transform;
  glm::mat4 _projection;
};
} // namespace AltheaEngine