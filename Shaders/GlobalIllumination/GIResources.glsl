#ifndef _GIRESOURCES_
#define _GIRESOURCES_

#define EMPTY_SLOT 0
#define RESERVED_SLOT 0xFFFFFFFF

#include <Misc/Hash.glsl>

#include <Bindless/GlobalHeap.glsl>
#include <Global/GlobalResources.glsl>
#include <Global/GlobalUniforms.glsl>
#include <PrimitiveResources.glsl>
#include <Misc/Sampling.glsl>

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : enable

layout(push_constant) uniform PushConstant {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
  uint giUniformsHandle;
} pushConstants;

UNIFORM_BUFFER(_giUniforms, GIUniforms{
  uint tlas;
  
  uint colorSamplers[2];
  uint colorTargets[2];
  uint depthSamplers[2];
  uint depthTargets[2];
  
  uint targetWidth;
  uint targetHeight;
  
  uint writeIndex;
  
  uint reservoirHeap;
  uint reservoirsPerBuffer;

  uint framesSinceCameraMoved;
});
#define giUniforms _giUniforms[pushConstants.giUniformsHandle]

SAMPLER2D(textureHeap);

#define resources RESOURCE(globalResources, pushConstants.globalResourcesHandle)
#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)

#define environmentMap textureHeap[resources.ibl.environmentMapHandle]
#define prefilteredMap textureHeap[resources.ibl.prefilteredMapHandle]
#define irradianceMap textureHeap[resources.ibl.irradianceMapHandle]
#define brdfLut textureHeap[resources.ibl.brdfLutHandle]

IMAGE2D_W(imageHeap);

#define prevColorTargetTx textureHeap[giUniforms.colorSamplers[giUniforms.writeIndex^1]]
#define colorTargetTx textureHeap[giUniforms.colorSamplers[giUniforms.writeIndex]]

#define prevColorTargetImg imageHeap[giUniforms.colorTargets[giUniforms.writeIndex^1]]
#define colorTargetImg imageHeap[giUniforms.colorTargets[giUniforms.writeIndex]]

#define prevDepthTargetTx textureHeap[giUniforms.depthSamplers[giUniforms.writeIndex^1]]
#define depthTargetTx textureHeap[giUniforms.depthSamplers[giUniforms.writeIndex]]

#define prevDepthTargetImg imageHeap[giUniforms.depthTargets[giUniforms.writeIndex^1]]
#define depthTargetImg imageHeap[giUniforms.depthTargets[giUniforms.writeIndex]]

#ifndef IS_RT_SHADER
#define IS_RT_SHADER 1
#endif 

#if IS_RT_SHADER
TLAS(tlasHeap);
#define acc tlasHeap[giUniforms.tlas]
#endif 

struct GISample {
  vec3 dir;
  float W;
  vec3 radiance;
  float padding; 
};

struct Reservoir {
  GISample samples[8];
  uint sampleCount;
  float wSum;
};

BUFFER_RW(_reservoirBuffer, ReservoirBuffer{
  Reservoir reservoirs[];
});

// The reservoir heap is split into sub-buffers with sequential bindless indices. 
// The first sub-buffer is given by giUniforms.reservoirHeap and each buffer
// has giUniforms.reservoirsPerBuffer entries.
#define getReservoir(reservoirIdx)                    \
    _reservoirBuffer[                                 \
      giUniforms.reservoirHeap +                      \
      reservoirIdx / giUniforms.reservoirsPerBuffer]  \
        .reservoirs[                                  \
          reservoirIdx % giUniforms.reservoirsPerBuffer]

#define INTERPOLATE(member)(v.member=v0.member*bc.x+v1.member*bc.y+v2.member*bc.z)


vec3 integrateReservoir(uint reservoirIdx) {
  vec3 irradiance = vec3(0.0);
  float wSum = getReservoir(reservoirIdx).wSum;
  for (int i = 0; i < 8; ++i) {
    irradiance += 
        0.125 * getReservoir(reservoirIdx).samples[i].radiance 
        * getReservoir(reservoirIdx).samples[i].W / wSum;
  }

  return irradiance;
}

vec3 sampleReservoirWeighted(uint reservoirIdx, inout uvec2 seed) {
  float x = rng(seed) * getReservoir(reservoirIdx).wSum;
  float p = 0.0;
  for (int i = 0; i < 8; ++i) {
    p += getReservoir(reservoirIdx).samples[i].W;
    if (x <= p || i == 7)
      return getReservoir(reservoirIdx).samples[i].radiance;
  }
}

vec3 sampleReservoirUniform(uint reservoirIdx, inout uvec2 seed) {
  float x = rng(seed);
  uint sampleIdx = uint(8.0 * x) % 8;
  return getReservoir(reservoirIdx).samples[sampleIdx].radiance;
}

vec2 reprojectToPrevFrameUV(vec4 pos) {
  vec4 prevScrUvH = globals.projection * globals.prevView * pos;
  return prevScrUvH.xy / prevScrUvH.w * 0.5 + vec2(0.5);
}
#endif // _GIRESOURCES_