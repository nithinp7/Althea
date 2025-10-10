#ifndef _COMMONTRANSLATIONS_
#define _COMMONTRANSLATIONS_

#ifndef IS_SHADER
#include <cstdint>
#include <glm/glm.hpp>

using namespace glm;

#define uint uint32_t
#endif

#ifdef IS_SHADER

#ifdef IS_HLSL_SHADER
#define uvec4 uint4
#define uvec3 uint3
#define uvec2 uint2
#define ivec4 int4
#define ivec3 int3
#define ivec2 int2
#define vec4 float4
#define vec3 float3
#define vec2 float2
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#endif // IS_HLSL_SHADER
// packed formats
#define u16vec4 uvec2
uvec4 unpack_u16vec4(uvec2 packed) { 
  return uvec4(packed.x >> 16, packed.x & 0xffff, packed.y >> 16, packed.y & 0xffff);
  // return uvec4(packed.x & 0xffff, packed.x >> 16, packed.y & 0xffff, packed.y >> 16);
}
#endif

#endif // _COMMONTRANSLATIONS_