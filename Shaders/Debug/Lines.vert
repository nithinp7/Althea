#version 450

layout(location=0) out vec3 worldPosOut;
layout(location=1) out vec3 colorOut;

// TODO: Move these to an include
struct DebugLineVertex {
  vec3 worldPos;
  uint color;
};

#extension GL_EXT_nonuniform_qualifier : enable

layout(std430, set=0, location=DEBUG_BUFFER_BINDING) buffer DBG_LINE_HEAP {
  DebugLineVertex vertices[];
} debugLineHeap[];

layout(push_constant) uniform PushConstants {
  uint bufferIdx;
  uint primOffs;
} pushConstants;

void main() {
  DebugLineVertex vertex = 
      debugLineHeap[pushConstants.bufferIdx]
      .vertices[2 * primOffs + gl_VertexIndex];

  worldPosOut = vertex.worldPos;
  colorOut = 
      vec3(
        (vertex.color >> 16) & 0xFF, 
        (vertex.color >> 8) & 0xFF, 
        vertex.color & 0xFF) / 255.0
}