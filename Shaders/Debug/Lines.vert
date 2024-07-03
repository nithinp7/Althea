#version 450

layout(location=0) in vec3 inWorldPos;
layout(location=1) in uint inColor;

layout(location=0) out vec3 outNormal;
layout(location=1) out vec3 outColor;

#include "DebugDraw.glsl"

void main() {
  gl_Position = globals.projection * globals.view * vec4(inWorldPos, 1.0);

  outNormal = normalize(globals.view[3].xyz - inWorldPos);
  outColor = 
      vec3(
        (inColor >> 24) & 0xFF, 
        (inColor >> 16) & 0xFF, 
        (inColor >> 8) & 0xFF) / 255.0;
}