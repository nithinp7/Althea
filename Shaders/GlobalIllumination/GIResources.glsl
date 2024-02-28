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

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

#define gBufferDepth textureHeap[resources.gBuffer.depthHandle]
#define gBufferNormal textureHeap[resources.gBuffer.normalHandle]
#define gBufferAlbedo textureHeap[resources.gBuffer.albedoHandle]
#define gBufferMetallicRoughnessOcclusion textureHeap[resources.gBuffer.metallicRoughnessOcclusionHandle]

vec3 reconstructPosition(vec2 uv) {
  float dRaw = texture(gBufferDepth, uv).r;

  // TODO: Stop hardcoding this
  float near = 0.01;
  float far = 1000.0;
  float d = far * near / (dRaw * (far - near) - far);

  vec2 ndc = 2.0 * uv - vec2(1.0);

  vec4 camPlanePos = vec4(ndc, 2.0, 1.0);
  vec4 dirH = globals.inverseProjection * camPlanePos;
  dirH.xyz /= dirH.w;
  dirH.w = 0.0;
  dirH = globals.inverseView * dirH;
  vec3 dir = normalize(dirH.xyz);

  float f = dot(dir, globals.inverseView[2].xyz);

  return globals.inverseView[3].xyz + d * dir / f;
}

IMAGE2D_W(imageHeap);

#define prevColorTargetTx textureHeap[giUniforms.colorSamplers[giUniforms.writeIndex^1]]
#define colorTargetTx textureHeap[giUniforms.colorSamplers[giUniforms.writeIndex]]

#define prevColorTargetImg imageHeap[giUniforms.colorTargets[giUniforms.writeIndex^1]]
#define colorTargetImg imageHeap[giUniforms.colorTargets[giUniforms.writeIndex]]

#ifndef IS_RT_SHADER
#define IS_RT_SHADER 1
#endif 

#if IS_RT_SHADER
TLAS(tlasHeap);
#define acc tlasHeap[giUniforms.tlas]
#endif // IS_RT_SHADER

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

int sampleReservoirIndexWeighted(uint reservoirIdx, inout uvec2 seed) {
  float x = rng(seed) * getReservoir(reservoirIdx).wSum;
  float p = 0.0;
  uint sampleCount = getReservoir(reservoirIdx).sampleCount;
  for (int i = 0; i < sampleCount; ++i) {
    p += getReservoir(reservoirIdx).samples[i].W;
    if (x <= p || i == sampleCount)
      return i;
  }

  return -1;
}

vec3 sampleReservoirWeighted(uint reservoirIdx, inout uvec2 seed) {
  int sampleIdx = sampleReservoirIndexWeighted(reservoirIdx, seed);
  return getReservoir(reservoirIdx).samples[sampleIdx].radiance;
}

int sampleReservoirIndexInverseWeighted(uint reservoirIdx, inout uvec2 seed) {
  uint sampleCount = getReservoir(reservoirIdx).sampleCount;
  float wSum = getReservoir(reservoirIdx).wSum;
  float x = rng(seed) * (float(sampleCount) - 1.0);
  float p = 0.0;
  for (int i = 0; i < sampleCount; ++i) {
    p += 1.0 - getReservoir(reservoirIdx).samples[i].W / wSum;
    if (x <= p || i == 7)
      return i;
  }

  return -1;
}

int sampleReservoirIndexUniform(uint reservoirIdx, inout uvec2 seed) {
  float x = rng(seed);
  uint sampleCount = getReservoir(reservoirIdx).sampleCount;
  uint sampleIdx = uint(8.0 * x) % sampleCount;
  return int(sampleIdx); 
}

vec3 sampleReservoirUniform(uint reservoirIdx, inout uvec2 seed) {
  int sampleIdx = sampleReservoirIndexUniform(reservoirIdx, seed);
  return getReservoir(reservoirIdx).samples[sampleIdx].radiance;
}

vec2 reprojectToPrevFrameUV(vec4 pos) {
  vec4 prevScrUvH = globals.projection * globals.prevView * pos;
  return prevScrUvH.xy / prevScrUvH.w * 0.5 + vec2(0.5);
}

vec2 projectToUV(vec4 pos) {
  vec4 scrUvH = globals.projection * globals.view * pos;
  return scrUvH.xy / scrUvH.w * 0.5 + vec2(0.5);
}

bool isValidUV(vec2 uv) {
  return
      uv.x >= 0.0 && uv.x <= 1.0 && 
      uv.y >= 0.0 && uv.y <= 1.0;
}

#if IS_RT_SHADER
ivec2 getClosestPixel(vec2 uv) {
  vec2 pixelPosF = uv * vec2(gl_LaunchSizeEXT); 
  ivec2 pixelPos = ivec2(pixelPosF);
  if (pixelPos.x < 0)
    pixelPos.x = 0;
  if (pixelPos.x >= gl_LaunchSizeEXT.x)
    pixelPos.x = int(gl_LaunchSizeEXT.x-1);
  if (pixelPos.y < 0)
    pixelPos.y = 0;
  if (pixelPos.y >= gl_LaunchSizeEXT.y)
    pixelPos.y = int(gl_LaunchSizeEXT.y-1);
    
  return pixelPos;
}
#endif // IS_RT_SHADER

bool validateColor(inout vec3 color) {
  if (color.x < 0.0 || color.y < 0.0 || color.z < 0.0) {
    color = vec3(1000.0, 0.0, 0.0);
    return false;
  }

  if (isnan(color.x) || isnan(color.y) || isnan(color.z)) {
    color = vec3(0.0, 1000.0, 0.0);
    return false;
  }  
  
  return true;
}

#endif // _GIRESOURCES_