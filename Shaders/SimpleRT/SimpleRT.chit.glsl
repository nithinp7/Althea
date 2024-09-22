// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#define IS_RT_SHADER
#include <SimpleRT/SimpleRTResources.glsl>
#include <PathTracing/BRDF.glsl>

layout(location = 0) rayPayloadInEXT RtPayload payload;
hitAttributeEXT vec2 attribs;

#define INTERPOLATE(member)(v.member=v0.member*bc.x+v1.member*bc.y+v2.member*bc.z)

void main() {
    BINDLESS(primitiveConstants, gl_InstanceCustomIndexEXT);
    PrimitiveConstants primInfo = GET_CONSTANTS(primitiveConstants);
    
    BINDLESS(primitiveVertices, primInfo.vertexBufferHandle);
    BINDLESS(primitiveIndices, primInfo.indexBufferHandle);
    
    vec3 bc = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    uint idx0 = BUFFER_GET(primitiveIndices, 3*gl_PrimitiveID+0);
    uint idx1 = BUFFER_GET(primitiveIndices, 3*gl_PrimitiveID+1);
    uint idx2 = BUFFER_GET(primitiveIndices, 3*gl_PrimitiveID+2);

    Vertex v0 = BUFFER_GET(primitiveVertices, idx0);
    Vertex v1 = BUFFER_GET(primitiveVertices, idx1);
    Vertex v2 = BUFFER_GET(primitiveVertices, idx2);

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
    
    vec4 baseColor = texture(
        samplerHeap[primInfo.baseTextureHandle], 
        v.uvs[primInfo.baseTextureCoordinateIndex]) * primInfo.baseColorFactor;

    vec3 normalMapSample = texture(
        samplerHeap[primInfo.normalTextureHandle], 
        v.uvs[primInfo.baseTextureCoordinateIndex]).rgb;
    vec3 tangentSpaceNormal = 
        (2.0 * normalMapSample - 1.0) *
        vec3(primInfo.normalScale, primInfo.normalScale, 1.0);
    vec3 globalNormal = 
        normalize(gl_ObjectToWorldEXT *
        vec4(normalize(
          tangentSpaceNormal.x * v.tangent + 
          tangentSpaceNormal.y * v.bitangent + 
          tangentSpaceNormal.z * v.normal), 0.0));

    vec2 metallicRoughness = texture(
        samplerHeap[primInfo.metallicRoughnessTextureHandle], 
        v.uvs[primInfo.metallicRoughnessTextureCoordinateIndex]).bg *
          vec2(primInfo.metallicFactor, primInfo.roughnessFactor);
    // float ambientOcclusion = 
    //     texture(occlusionTexture(primIdx), v.uvs[primInfo.occlusionTextureCoordinateIndex]).r * primInfo.occlusionStrength;

    payload.color = vec4(fract(globalNormal), 1.0);

    vec3 wiw;
    float pdf;
    vec3 f = sampleMicrofacetBrdf(
      payload.xi, -payload.dir, globalNormal, 
      baseColor.rgb, metallicRoughness.x, metallicRoughness.y, 
      wiw, pdf);

    payload.color = vec4(f * sampleEnvMap(wiw) / pdf, 1.0);
}