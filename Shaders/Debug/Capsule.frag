
#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inColor;

layout(location=0) out vec4 GBuffer_Normal;
layout(location=1) out vec4 GBuffer_Albedo;
layout(location=2) out vec4 GBuffer_MetallicRoughnessDebug;

void main() {
  GBuffer_Albedo = vec4(inColor, 1.0);
#ifndef WIREFRAME
  if (inNormal == vec3(0.0)) {
    vec3 dFdxPos = dFdx(inPosition);
    vec3 dFdyPos = dFdy(inPosition);
    GBuffer_Normal = vec4(normalize(-cross(dFdxPos,dFdyPos)), 1.0);
  } else 
#endif
  {
    GBuffer_Normal = vec4(inNormal, 1.0);
  }
  GBuffer_MetallicRoughnessDebug = vec4(0.0, 0.3, 0.0, 1.0);
}
