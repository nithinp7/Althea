#pragma once

#include <glm/mat4x4.hpp>

namespace AltheaEngine {
struct ModelViewProjection {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};
} // namespace AltheaEngine
