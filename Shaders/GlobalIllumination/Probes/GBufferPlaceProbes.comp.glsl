#version 460

#define IS_RT_SHADER 0
#include <GlobalIllumination/GIResources.glsl>
#include <Misc/Input.glsl>

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
void main() {
  vec2 uv = (vec2(gl_GlobalInvocationID.xy) + vec2(0.5)) / vec2(gl_WorkGroupSize.xy * gl_NumWorkGroups.xy);
  if (!isValidUV(uv))
    return;

  if (bool(globals.inputMask & INPUT_BIT_CTRL) &&
      bool(globals.inputMask & INPUT_BIT_LEFT_MOUSE)) {
    probesController.instanceCount = 0;
    return;
  }

  // Attempt to allocate probe for placement
  uint probeIdx = atomicAdd(probesController.instanceCount, 1);
  // incoherently clamp probe count if out of probes
  if (probeIdx >= PROBE_COUNT) {
    probesController.instanceCount = PROBE_COUNT;
    return;
  }
  
  // Fetch world pos from depth
  vec3 normal = texture(gBufferNormal, uv).rgb;
  vec3 position = reconstructPosition(uv) + normal;

  getProbe(probeIdx).position = position;
}