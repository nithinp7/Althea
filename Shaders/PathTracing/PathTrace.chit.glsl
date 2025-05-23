// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"

layout(location = 0) rayPayloadInEXT PathTracePayload payload;
hitAttributeEXT vec2 attribs;
layout(location = 1) rayPayloadEXT PathTracePayload shadowRayPayload;

#include "BRDF.glsl"

#define primInfo getPrimitive(primIdx)

void main() {
    vec3 bc = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    uint primIdx = uint(gl_InstanceCustomIndexEXT);

    uint idx0 = getIndex(primIdx, 3*gl_PrimitiveID+0);
    uint idx1 = getIndex(primIdx, 3*gl_PrimitiveID+1);
    uint idx2 = getIndex(primIdx, 3*gl_PrimitiveID+2);

    Vertex v0 = getVertex(primIdx, idx0);
    Vertex v1 = getVertex(primIdx, idx1);
    Vertex v2 = getVertex(primIdx, idx2);

    Vertex v;
    INTERPOLATE(position);
    INTERPOLATE(tangent);
    INTERPOLATE(bitangent);
    INTERPOLATE(normal);
    INTERPOLATE(uvs[0]);
    INTERPOLATE(uvs[1]);
    INTERPOLATE(uvs[2]);
    INTERPOLATE(uvs[3]);

    vec3 worldPos = gl_ObjectToWorldEXT * vec4(v.position, 1.0);
    vec4 baseColor = 
        texture(baseColorTexture(primIdx), v.uvs[primInfo.baseTextureCoordinateIndex]) * primInfo.baseColorFactor;

    vec3 normalMapSample = texture(normalMapTexture(primIdx), v.uvs[primInfo.baseTextureCoordinateIndex]).rgb;
    vec3 tangentSpaceNormal = 
        (2.0 * normalMapSample - 1.0) *
        vec3(primInfo.normalScale, primInfo.normalScale, 1.0);
    vec3 globalNormal = 
        normalize(gl_ObjectToWorldEXT *
        vec4(normalize(
          tangentSpaceNormal.x * v.tangent + 
          tangentSpaceNormal.y * v.bitangent + 
          tangentSpaceNormal.z * v.normal), 0.0));

    vec2 metallicRoughness = 
        texture(metallicRoughnessTexture(primIdx), v.uvs[primInfo.metallicRoughnessTextureCoordinateIndex]).bg *
        vec2(primInfo.metallicFactor, primInfo.roughnessFactor);
    // float ambientOcclusion = 
    //     texture(occlusionTexture(primIdx), v.uvs[primInfo.occlusionTextureCoordinateIndex]).r * primInfo.occlusionStrength;

    payload.p = vec4(worldPos, 1.0);
    payload.n = globalNormal;

    payload.baseColor = baseColor.rgb;
    payload.emissive = texture(emissiveTexture(primIdx), v.uvs[primInfo.emissiveTextureCoordinateIndex]).rgb * primInfo.emissiveFactor.rgb;
    payload.metallic = metallicRoughness.x;
    payload.roughness = metallicRoughness.y;
}