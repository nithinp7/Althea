#version 450

// inst data
layout(location=0) in vec3 inA;
layout(location=1) in vec3 inB;
layout(location=2) in float inRadius;
layout(location=3) in uint inColor;

// vert data
layout(location=4) in vec3 inVertPos;

layout(location=0) out vec3 outNormal;
layout(location=1) out vec3 outColor;

#include "DebugDraw.glsl"

#define isPhaseSphereA (pushConstants.extras0 == 0)
#define isPhaseSphereB (pushConstants.extras0 == 1)
#define isPhaseCylinder (pushConstants.extras0 == 2)

void main() {
  vec3 worldPos = vec3(0.0);
  if (isPhaseSphereA) {
    worldPos = inA + inVertPos * inRadius;
  } else if (isPhaseSphereB) {
    worldPos = inB + inVertPos * inRadius;
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

    worldPos = 
        inRadius * inVertPos.x * perp + 
        inRadius * inVertPos.y * perp2 +
        mix(inA, inB, inVertPos.z);
  }

  gl_Position = globals.projection * globals.view * vec4(worldPos, 1.0);

  outNormal = normalize(globals.view[3].xyz - worldPos);
  outColor = 
      vec3(
        (inColor >> 24) & 0xFF, 
        (inColor >> 16) & 0xFF, 
        (inColor >> 8) & 0xFF) / 255.0;
}