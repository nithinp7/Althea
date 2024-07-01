#ifndef IS_SHADER
#include <cstdint>
#include <glm/glm.hpp>

using namespace glm;

#define uint uint32_t
#endif

#ifdef IS_SHADER
// packed formats
#define u16vec4 uvec2
uvec4 unpack_u16vec4(uvec2 packed) { 
  return uvec4(packed.x >> 16, packed.x & 0xffff, packed.y >> 16, packed.y & 0xffff);
  // return uvec4(packed.x & 0xffff, packed.x >> 16, packed.y & 0xffff, packed.y >> 16);
}
#endif