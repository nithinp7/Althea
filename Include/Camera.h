#pragma once

#include "Library.h"

#include "Library.h"

#include <glm/glm.hpp>

namespace AltheaEngine {
class ALTHEA_API Camera {
public:
  Camera(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);
  void setFovDegrees(float fovDegrees);
  void setAspectRatio(float aspectRatio);
  void setClippingPlanes(float nearPlane, float farPlane);
  void setPosition(const glm::vec3& position);
  void setRotation(float yawDegrees, float pitchDegrees);

  const glm::mat4& getTransform() const { return this->_transform; }

  float computeYawDegrees() const;
  float computePitchDegrees() const;
  glm::mat4 computeView() const;

  const glm::mat4& getProjection() const { return this->_projection; }

private:
  void _recomputeProjection();

  float _fov;
  float _aspectRatio;
  float _nearPlane;
  float _farPlane;
  glm::mat4 _transform;
  glm::mat4 _projection;
};
} // namespace AltheaEngine