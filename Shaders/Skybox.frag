#version 450

layout(location=0) smooth in vec3 direction;

layout(location=0) out vec4 color;

layout(set=0, binding=0) uniform samplerCube skyboxTexture; 

void main() {
  color = texture(skyboxTexture, direction);
}