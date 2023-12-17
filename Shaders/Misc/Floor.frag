
#version 450

layout(location=0) in vec3 direction;
// layout(location=1) in vec2 screenUV;

layout(location=0) out vec4 GBuffer_Position;
layout(location=1) out vec4 GBuffer_Normal;
layout(location=2) out vec4 GBuffer_Albedo;
layout(location=3) out vec4 GBuffer_MetallicRoughnessOcclusion;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

void main() {
  // vec2 ndc = 2.0 * screenUV - vec2(1.0);
  float floorHeight = 0.0;

  vec3 cameraPos = globals.inverseView[3].xyz;
  vec3 dir = normalize(direction);

  // Solve for t to find whether and where the camera ray intersects the
  // floor plane
  // c.y + t * d.y = 0

  float t = -1.0;
  if (abs(dir.y) > 0.0001)
  {
    t = floorHeight - cameraPos.y / dir.y;
  } 

  float zNear = 0.01;
  float zFar = 1000.0;
  if (t < zNear)
  {
    discard;
  } else {
    // t += zNear;

    // linearize: 
    // lin = zNear * zFar / (zFar + d * (zNear - zFar))

    // d  = (zNear * zFar - lin * zFar) / (lin * (zNear - zFar))
    float d = zFar * (zNear - t) / (t * (zNear - zFar));
    gl_FragDepth = d;

    vec4 floorPos = vec4(cameraPos + dir * t, 1.0);
    // vec4 clipSpace = globals.projection * globals.view * floorPos;
    // float d = clipSpace.z / clipSpace.w;

    // // TODO: ???
    // gl_FragDepth = d;

    GBuffer_Position = floorPos;
    GBuffer_Normal = vec4(0.0, 1.0, 0.0, 1.0);
    GBuffer_Albedo = vec4(0.9, 0.05, 0.1, 1.0);
    GBuffer_MetallicRoughnessOcclusion = vec4(0.0, 0.04, 1.0, 1.0);
  }
}
