
#version 460 core

#extension GL_EXT_ray_query : enable
// #extension GL_EXT_maintenance4 : enable
// #extension GL_EXT_ray_tracing : enable

layout(local_size_x = 32) in;

#include <GlobalIllumination/GIResources.glsl>
#include <Misc/Constants.glsl>

#define TARGET_WIDTH 128

void main() {
  uint idx = uint(gl_GlobalInvocationID.x);
  ivec2 pixelPos = ivec2(idx / TARGET_WIDTH, idx % TARGET_WIDTH);
  // if (pixelPos.x >= TARGET_WIDTH) {
  //   return;
  // }

  vec2 uv = (vec2(pixelPos) + vec2(0.5)) / float(TARGET_WIDTH);
  vec2 thetaPhi = uv;
  thetaPhi.y -= 0.5;
  thetaPhi *= TWO_PI;

  float cosTheta = cos(thetaPhi.x);
  float sinTheta = sin(thetaPhi.x);
  float cosPhi = cos(thetaPhi.y);
  float sinPhi = sin(thetaPhi.y);

  vec3 worldPos = vec3(0.0);
  vec3 dir = vec3(cosTheta * cosPhi, sinPhi, sinTheta * cosPhi);

  rayQueryEXT query;
	rayQueryInitializeEXT(
        query, 
        acc, 
        gl_RayFlagsTerminateOnFirstHitEXT, 
        0xFF, 
        worldPos, 
        0.01, 
        dir, 
        1000.0);
  rayQueryProceedEXT(query);

  float dist = 1000.0;
  if (rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
  {
    dist = rayQueryGetIntersectionTEXT(query, true);
  }

  vec3 outColor = vec3(dist) / 1000.0;//0.5 * dir + vec3(0.5);
  
  imageStore(debugTarget, pixelPos, vec4(outColor, 1.0));
}
