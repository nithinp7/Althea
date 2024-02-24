
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
  vec2 scrUv = vec2(pixelPos) / vec2(gl_LaunchSizeEXT);

  vec4 gb_pos = texture(gBufferPosition, scrUv);
  if (gb_pos.a == 0.0)
    return vec3(0.0);
  
  vec3 gb_normal = texture(gBufferNormal, scrUv).rgb;
  vec3 gb_albedo = texture(gBufferAlbedo, scrUv).rgb;
  vec3 gb_metallicRoughnessOcclusion = texture(gBufferMetallicRoughnessOcclusion, scrUv).rgb;

  payload.seed = uvec2(gl_LaunchIDEXT) * uvec2(giUniforms.framesSinceCameraMoved+1, giUniforms.framesSinceCameraMoved+2);

  vec3 wow = normalize(globals.inverseView[3].xyz - gb_pos.xyz);
  // TODO:
  vec3 wiw = reflect(-wow, gb_normal);
  vec3 p = gb_pos.xyz + wiw * 0.01;

  vec3 color = vec3(0.0);
  {
    payload.o = p;
    payload.wow = -wiw;
    traceRayEXT(
        acc, 
        gl_RayFlagsOpaqueEXT, 
        0xff, 
        0, // sbtOffset
        0, // sbtStride, 
        0, // missIndex
        p, 
        0.0,
        wiw,
        1000.0, 
        0 /* payload */);

    if (payload.p.w > 0.0)
    {
      p = payload.p.xyz / payload.p.w;
      color = payload.baseColor;
    }
  }

  return color;
}

void main() {
  ivec2 pixelPos = ivec2(gl_LaunchIDEXT);

  vec3 color = trace();
  validateColor(color);
  imageStore(colorTargetImg, pixelPos, vec4(color, 1.0));
}