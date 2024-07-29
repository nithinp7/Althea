#version 450

// inst data
layout(location=0) in mat4 inModel;
layout(location=4) in vec3 inA;
layout(location=5) in vec3 inB;
layout(location=6) in float inRadius;
layout(location=7) in uint inColor;

// vert data
layout(location=8) in vec3 inVertPos;

layout(location=0) out vec3 outPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec3 outColor;

#include "DebugDraw.glsl"

#define isPhaseSphereA (pushConstants.extras0 == 0)
#define isPhaseSphereB (pushConstants.extras0 == 1)
#define isPhaseCylinder (pushConstants.extras0 == 2)

void main() {
  vec3 restPose = vec3(0.0);
  if (isPhaseSphereA) {
    restPose = inA + inVertPos * inRadius;
  } else if (isPhaseSphereB) {
    restPose = inB + inVertPos * inRadius;
  } else if (isPhaseCylinder) {
    // generate local crs
    vec3 diff = inB - inA;
    vec3 perp = cross(diff, vec3(0.0, 1.0, 0.0));
    float perpDistSq = dot(perp, perp);
    if (perpDistSq < 0.001)
      perp = normalize(cross(diff, vec3(1.0, 0.0, 0.0)));
    else 
      perp /= sqrt(perpDistSq);
    vec3 perp2 = normalize(cross(diff, perp));

    restPose = 
        inRadius * inVertPos.x * perp + 
        inRadius * inVertPos.y * perp2 +
        mix(inA, inB, inVertPos.z);
  }

  vec4 worldPos = inModel * vec4(restPose, 1.0);
  gl_Position = globals.projection * globals.view * worldPos;

  outPosition = worldPos.xyz;
#ifdef WIREFRAME
  outNormal = normalize(globals.inverseView[3].xyz - worldPos.xyz);
#else
  outNormal = vec3(0.0);
#endif
  outColor = 
      vec3(
        (inColor >> 24) & 0xFF, 
        (inColor >> 16) & 0xFF, 
        (inColor >> 8) & 0xFF) / 255.0;
}