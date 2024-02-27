
// Ray generation shader
#version 460 core

#extension GL_EXT_ray_tracing : enable

#include <GlobalIllumination/GIResources.glsl>

#include <Misc/Sampling.glsl>
#include "PathTracePayload.glsl"
#include "BRDF.glsl"

layout(location = 0) rayPayloadEXT PathTracePayload payload;

bool isValidUV(vec2 uv) {
  return
      uv.x >= 0.0 && uv.x <= 1.0 && 
      uv.y >= 0.0 && uv.y <= 1.0;
}

ivec2 getClosestPixel(vec2 uv) {
  vec2 pixelPosF = uv * vec2(gl_LaunchSizeEXT); 
  ivec2 pixelPos = ivec2(pixelPosF);
  if (pixelPos.x < 0)
    pixelPos.x = 0;
  if (pixelPos.x >= gl_LaunchSizeEXT.x)
    pixelPos.x = int(gl_LaunchSizeEXT.x-1);
  if (pixelPos.y < 0)
    pixelPos.y = 0;
  if (pixelPos.y >= gl_LaunchSizeEXT.y)
    pixelPos.y = int(gl_LaunchSizeEXT.y-1);
    
  return pixelPos;
}

bool validateColor(inout vec3 color) {
  if (color.x < 0.0 || color.y < 0.0 || color.z < 0.0) {
    color = vec3(1000.0, 0.0, 0.0);
    return false;
  }

  if (isnan(color.x) || isnan(color.y) || isnan(color.z)) {
    color = vec3(0.0, 1000.0, 0.0);
    return false;
  }  
  
  return true;
}

vec3 trace() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);
  vec2 jitter = vec2(0.5);//randVec2(payload.seed);
  vec2 scrUv = (vec2(pixelPos) + jitter) / vec2(gl_LaunchSizeEXT);

  vec2 ndc = scrUv * 2.0 - 1.0;
  vec4 camPlanePos = globals.inverseProjection * vec4(ndc.x, ndc.y, 1, 1);
  vec3 wow = -(globals.inverseView*vec4(normalize(camPlanePos.xyz), 0)).xyz;

  vec4 gb_albedo = texture(gBufferAlbedo, scrUv);
  if (gb_albedo.a == 0.0) {
    return sampleEnvMap(-wow);
  }
  
  vec3 position = reconstructPosition(scrUv);
  vec3 gb_normal = normalize(texture(gBufferNormal, scrUv).rgb);
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

#if 0
  vec3 wiw = reflect(-wow, gb_normal);
  float pdf = 1.0;
  vec3 f = vec3(0.0);
#else 
  vec3 wiw;
  float pdf;
  vec3 f = 
      sampleMicrofacetBrdf(
        randVec2(payload.seed),
        wow,
        gb_normal,
        gb_albedo.rgb,
        gb_metallicRoughnessOcclusion.x,
        gb_metallicRoughnessOcclusion.y,
        wiw,
        pdf);
#endif
  // return fract(0.1 * position.xyz);
  vec3 p = position + wiw * 0.1;

  vec3 color = vec3(0.0);
  if (pdf > 0.0001)
  {
    payload.o = p;
    payload.wow = -wiw;
    traceRayEXT(
        acc, 
        gl_RayFlagsOpaqueEXT, 
        // gl_RayFlagsCullBackFacingTrianglesEXT,
        0xff, 
        0, // sbtOffset
        0, // sbtStride, 
        0, // missIndex
        p, 
        0.0,
        wiw,
        1000.0, 
        0 /* payload */);

#if 0
    if (payload.p.w > 0.0)
    {
      // p = payload.p.xyz / payload.p.w;
      color = payload.baseColor + payload.emissive;
    } else {
      color = payload.emissive;
    }
#else 
    if (payload.emissive != vec3(0.0))
    {
      color = f * max(dot(wiw, gb_normal), 0.0) * payload.emissive / pdf;
    }
#endif
  }

  return color;
}

void main() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);

  vec3 color = trace();
  // validateColor(color);
  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}