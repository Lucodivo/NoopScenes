#version 410
layout (location = 0) in vec3 inTexCoord;

uniform samplerCube skybox;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = texture(skybox, inTexCoord);
}