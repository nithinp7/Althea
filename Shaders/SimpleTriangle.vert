
#version 450

vec2 positions[3] = vec2[](
  vec2(0.0, -0.5),
  vec2(0.5, 0.5),
  vec2(-0.5, 0.5)
);

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColor;

void main() {
  gl_Position = vec4(mod(position.x, 1.0), mod(position.y, 1.0), 0.0, 1.0);
  fragColor = normal;
}