// Based on:
// https://www.khronos.org/blog/ray-tracing-in-vulkan

// Ray generation shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadEXT vec4 payload;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

#define POINT_LIGHTS_SET 0
#define POINT_LIGHTS_BINDING 5
#include <PointLights.glsl>

layout(set=0, binding=6) uniform samplerCubeArray shadowMapArray;

layout(set=0, binding=7) uniform sampler2D textureHeap[TEXTURE_HEAP_COUNT];

#define PRIMITIVE_CONSTANTS_SET 0
#define PRIMITIVE_CONSTANTS_BINDING 8
#include <PrimitiveConstants.glsl>

struct Vertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uvs[4];
};

#extension GL_EXT_scalar_block_layout : enable
layout(scalar, set=0, binding=9) readonly buffer VERTEX_BUFFER_HEAP { Vertex vertices[]; } vertexBufferHeap[VERTEX_BUFFER_HEAP_COUNT];
layout(set=0, binding=10) readonly buffer INDEX_BUFFER_HEAP { uint indices[]; } indexBufferHeap[INDEX_BUFFER_HEAP_COUNT];

// GBuffer textures
layout(set=1, binding=0) uniform sampler2D gBufferPosition;
layout(set=1, binding=1) uniform sampler2D gBufferNormal;
layout(set=1, binding=2) uniform sampler2D gBufferAlbedo;
layout(set=1, binding=3) uniform sampler2D gBufferMetallicRoughnessOcclusion;

#include <PBR/PBRMaterial.glsl>

layout(set=1, binding=4) uniform accelerationStructureEXT acc;

// Output image
layout(set=1, binding=5) uniform writeonly image2D img;

vec2 computeUv(uvec3 launchID, uvec3 launchSize) {
  const vec2 pixelCenter = vec2(launchID.xy) + vec2(0.5);
	return pixelCenter/vec2(launchSize.xy);
}

vec3 computeDir(vec2 uv) {
	vec2 d = uv * 2.0 - 1.0;

	vec4 origin = globals.inverseView * vec4(0,0,0,1);
	vec4 target = globals.inverseProjection * vec4(d.x, d.y, 1, 1) ;
	return (globals.inverseView*vec4(normalize(target.xyz), 0)).xyz;
}

void main() {
  vec2 uv = computeUv(gl_LaunchIDEXT, gl_LaunchSizeEXT);
  vec3 dir = computeDir(uv);

  vec4 position = texture(gBufferPosition, uv).rgba;
  if (position.a == 0.0) {
    // Nothing in the GBuffer
    imageStore(img, ivec2(gl_LaunchIDEXT), vec4(0.0));
    return;
  }

  vec3 normal = normalize(texture(gBufferNormal, uv).xyz);
  vec3 reflectedDirection = reflect(normalize(dir), normal);

  traceRayEXT(
      acc, 
      gl_RayFlagsOpaqueEXT, 
      0xff, 
      0, // sbtOffset
      0, // sbtStride, 
      0, // missIndex
      position.xyz, 
      0.0,
      reflectedDirection,
      1000.0, 
      0 /* payload */);
  
  vec4 color = payload;

  imageStore(img, ivec2(gl_LaunchIDEXT), color);
}