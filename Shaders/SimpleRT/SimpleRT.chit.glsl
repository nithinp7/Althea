// Closest hit shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#define IS_RT_SHADER
#include <SimpleRT/SimpleRTResources.glsl>
#include <InstanceData/InstanceData.glsl>
#include <PathTracing/BRDF.glsl>

layout(location = 0) rayPayloadInEXT RtPayload payload;
hitAttributeEXT vec2 attribs;

void main() {    
  vec3 bc = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  uint primitiveHandle = gl_InstanceCustomIndexEXT;

  // object-space vertex
  Vertex v = fetchInterpolatedVertexIndexed(gl_PrimitiveID, bc, primitiveHandle);    
  MaterialDesc material = fetchMaterialFromPrimitive(v, primitiveHandle);

  // put bump-mapped normal in world-space  
  vec3 normalWS = normalize(gl_ObjectToWorldEXT * vec4(material.normal, 0.0)).xyz;

  vec3 wiw;
  float pdf;
  vec3 f = sampleMicrofacetBrdf(
    payload.xi, -payload.dir, material.normal,
    material.baseColor.rgb, material.metallic, material.roughness, 
    wiw, pdf);

  payload.color = vec4(f * sampleEnvMap(wiw) / pdf, 1.0);
}