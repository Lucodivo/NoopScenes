#version 420
layout (location = 0) in vec3 inTexCoord;

uniform samplerCube skybox;

layout (location = 0) out vec4 outColor;

void main()
{
  vec3 yIsUpTexCoord = vec3(inTexCoord.x, inTexCoord.z, -inTexCoord.y);
  outColor = texture(skybox, yIsUpTexCoord);
}