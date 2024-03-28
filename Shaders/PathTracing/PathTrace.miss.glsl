// Miss shader
#version 460 core
#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>
#include "PathTracePayload.glsl"

layout(location = 0) rayPayloadInEXT PathTracePayload payload;

#define PI 3.14159265359

void main() {
    payload.p = vec4(-payload.wow, 0.0);
    payload.n = vec3(0.0);

    payload.baseColor = vec3(0.0);
    payload.emissive = 
        giUniforms.liveValues.checkbox2 ? 
        vec3(0.0) : 
        sampleEnvMap(-payload.wow);
    payload.metallic = 0.0;
    payload.roughness = 0.0;
}