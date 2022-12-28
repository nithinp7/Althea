#version 450

layout(location=0) smooth in vec3 direction;

layout(location=0) out vec4 color;

layout(binding=1) uniform samplerCube skyboxTexture; 

void main() {
  color = texture(skyboxTexture, direction);
}