#pragma once

#include "Model.h"

#include <cstdint>
#include <vector>

namespace AltheaEngine {
struct Animation {
  Model* m_pModel = nullptr;
  uint32_t m_animationIdx = 0;
  float m_time = 0.0f;
  bool m_bLooping = false;
  bool m_bFinished = false; 
  
  void updateAnimation(float deltaTime);
};

class AnimationSystem {
public:
  AnimationSystem() = default;

  void startAnimation(Model* pModel, uint32_t animationIdx, bool bLooping);
  void stopAnimation(Model* pModel, uint32_t animationIdx);
  void stopModelAnimations(Model* pModel);
  void stopAllAnimations();

  void update(float deltaTime);

private:
  std::vector<Animation> m_activeAnimations;
};
} // namespace AltheaEngine