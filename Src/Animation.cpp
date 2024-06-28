#include "Animation.h"

#include "Containers/StackVector.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Animation.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>

namespace AltheaEngine {
void AnimationSystem::startAnimation(
    Model* pModel,
    uint32_t animationIdx,
    bool bLooping) {
  m_activeAnimations.push_back(
      Animation{pModel, animationIdx, 0.0f, bLooping, false});
}

void AnimationSystem::stopAnimation(Model* pModel, uint32_t animationIdx) {
  m_activeAnimations.erase(
      std::remove_if(
          m_activeAnimations.begin(),
          m_activeAnimations.end(),
          [&](const Animation& anim) {
            return anim.m_pModel == pModel &&
                   anim.m_animationIdx == animationIdx;
          }),
      m_activeAnimations.end());
}

void AnimationSystem::stopModelAnimations(Model* pModel) {
  m_activeAnimations.erase(
      std::remove_if(
          m_activeAnimations.begin(),
          m_activeAnimations.end(),
          [&](const Animation& anim) { return anim.m_pModel == pModel; }),
      m_activeAnimations.end());
}

void AnimationSystem::stopAllAnimations() { m_activeAnimations.clear(); }

void AnimationSystem::update(float deltaTime) {
  m_activeAnimations.erase(
      std::remove_if(
          m_activeAnimations.begin(),
          m_activeAnimations.end(),
          [&](const Animation& anim) { return anim.m_bFinished; }),
      m_activeAnimations.end());

  for (Animation& animation : m_activeAnimations)
    animation.updateAnimation(deltaTime);
}

void Animation::updateAnimation(float deltaTime) {
  struct NodeTransform {
    glm::quat rotation{};
    glm::vec3 translation{};
    glm::vec3 scale = glm::vec3(1.0f);
    bool targeted = false;
  };

  const CesiumGltf::Model& gltfModel = m_pModel->getGltfModel();
  ALTHEA_STACK_VECTOR(nodeTransforms, NodeTransform, gltfModel.nodes.size());
  nodeTransforms.resize(gltfModel.nodes.size());
  for (uint32_t i = 0; i < gltfModel.nodes.size(); ++i) {
    NodeTransform& transform = nodeTransforms[i];
    const CesiumGltf::Node& gltfNode = gltfModel.nodes[i];
    glm::mat4 translation(1.0);
    if (gltfNode.translation.size() == 3) {
      transform.translation = glm::vec3(
          static_cast<float>(gltfNode.translation[0]),
          static_cast<float>(gltfNode.translation[1]),
          static_cast<float>(gltfNode.translation[2]));
    }

    if (gltfNode.rotation.size() == 4) {
      transform.rotation[0] = static_cast<float>(gltfNode.rotation[0]);
      transform.rotation[1] = static_cast<float>(gltfNode.rotation[1]);
      transform.rotation[2] = static_cast<float>(gltfNode.rotation[2]);
      transform.rotation[3] = static_cast<float>(gltfNode.rotation[3]);
    }

    if (gltfNode.scale.size() == 3) {
      transform.scale[0] = static_cast<float>(gltfNode.scale[0]);
      transform.scale[1] = static_cast<float>(gltfNode.scale[1]);
      transform.scale[2] = static_cast<float>(gltfNode.scale[2]);
    }
  }

  const CesiumGltf::Animation& animation = gltfModel.animations[m_animationIdx];
  for (const CesiumGltf::AnimationChannel& channel : animation.channels) {
    const CesiumGltf::AnimationSampler& sampler =
        animation.samplers[channel.sampler];
    CesiumGltf::AccessorView<float> timeSamples(gltfModel, sampler.input);

    if (m_time > timeSamples[timeSamples.size() - 1]) {
      if (m_bLooping) {
        m_time -= timeSamples[timeSamples.size() - 1];
      } else {
        // animation over
        m_bFinished = true;
        return;
      }
    }

    uint32_t sampleIdx = 0;
    float td = 0.0f;
    float t = 0.0f;

    // TODO: track last used sample to avoid linear scan?
    for (; sampleIdx < timeSamples.size() - 1; ++sampleIdx) {
      float t0 = timeSamples[sampleIdx];
      float t1 = timeSamples[sampleIdx + 1];
      if (m_time >= t0 && m_time <= t1) {
        td = t1 - t0;
        t = (m_time - t0) / td;
        break;
      }
    }

    // TODO: Interpolate looping anims better...

    // Follows the same notation as glTF 2.0 Spec - Appendix C: Animation
    // Sampler Interpolation Modes
    NodeTransform& transform = nodeTransforms[channel.target.node];
    if (sampleIdx == (timeSamples.size() - 1) ||
        sampler.interpolation ==
            CesiumGltf::AnimationSampler::Interpolation::STEP) {
      if (channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::translation) {
        transform.translation = CesiumGltf::AccessorView<glm::vec3>(
            gltfModel,
            sampler.output)[sampleIdx];
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::rotation) {
        transform.rotation = CesiumGltf::AccessorView<glm::quat>(
            gltfModel,
            sampler.output)[sampleIdx];
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::scale) {
        transform.scale = CesiumGltf::AccessorView<glm::vec3>(
            gltfModel,
            sampler.output)[sampleIdx];
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::weights) {
        // TODO:
      }
    } else if (
        sampler.interpolation ==
        CesiumGltf::AnimationSampler::Interpolation::LINEAR) {

      if (channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::translation) {
        CesiumGltf::AccessorView<glm::vec3> translations(
            gltfModel,
            sampler.output);
        transform.translation =
            glm::mix(translations[sampleIdx], translations[sampleIdx + 1], t);
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::rotation) {
        CesiumGltf::AccessorView<glm::quat> rotations(
            gltfModel,
            sampler.output);
        transform.rotation =
            glm::slerp(rotations[sampleIdx], rotations[sampleIdx + 1], t);
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::scale) {
        CesiumGltf::AccessorView<glm::vec3> scales(gltfModel, sampler.output);
        transform.scale = glm::mix(scales[sampleIdx], scales[sampleIdx + 1], t);
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::weights) {
        // TODO:
      }
    } else {
      // cubic spline interpolation
      float t2 = t * t;
      float t3 = t2 * t;

      float A = (2.0f * t3 - 3.0f * t2 + 1.0f);
      float B = td * (t3 - 2.0f * t2 + t);
      float C = (-2.0f * t3 + 3 * t2);
      float D = td * (t3 - t2);

      if (channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::translation) {
        CesiumGltf::AccessorView<glm::vec3> translations(
            gltfModel,
            sampler.output);

        glm::vec3 vk = translations[3 * sampleIdx + 1];
        glm::vec3 bk = translations[3 * sampleIdx + 2];
        glm::vec3 ak_1 = translations[3 * sampleIdx + 3];
        glm::vec3 vk_1 = translations[3 * sampleIdx + 4];
        transform.translation = A * vk + B * bk + C * vk_1 + D * ak_1;
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::rotation) {
        CesiumGltf::AccessorView<glm::quat> rotations(
            gltfModel,
            sampler.output);

        glm::quat vk = rotations[3 * sampleIdx + 1];
        glm::quat bk = rotations[3 * sampleIdx + 2];
        glm::quat ak_1 = rotations[3 * sampleIdx + 3];
        glm::quat vk_1 = rotations[3 * sampleIdx + 4];
        transform.rotation =
            glm::normalize(A * vk + B * bk + C * vk_1 + D * ak_1);
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::scale) {
        CesiumGltf::AccessorView<glm::vec3> scales(gltfModel, sampler.output);

        glm::vec3 vk = scales[3 * sampleIdx + 1];
        glm::vec3 bk = scales[3 * sampleIdx + 2];
        glm::vec3 ak_1 = scales[3 * sampleIdx + 3];
        glm::vec3 vk_1 = scales[3 * sampleIdx + 4];
        transform.scale = A * vk + B * bk + C * vk_1 + D * ak_1;
      } else if (
          channel.target.path ==
          CesiumGltf::AnimationChannelTarget::Path::weights) {
        // TODO:
      }
    }

    transform.targeted = true;
  }

  for (uint32_t i = 0; i < gltfModel.nodes.size(); ++i) {
    const NodeTransform& transform = nodeTransforms[i];
    if (transform.targeted) {
      glm::mat4 relativeTransform(transform.rotation);
      relativeTransform[0] *= transform.scale.x;
      relativeTransform[1] *= transform.scale.y;
      relativeTransform[2] *= transform.scale.z;
      relativeTransform[3] = glm::vec4(transform.translation, 1.0f);

      m_pModel->setNodeRelativeTransform(i, relativeTransform);
    }
  }

  m_pModel->recomputeTransforms();

  m_time += deltaTime;
}

} // namespace AltheaEngine