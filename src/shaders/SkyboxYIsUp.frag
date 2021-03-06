#version 420
layout (location = 0) in vec3 inTexCoord;

uniform samplerCube skyboxTex;

layout (location = 0) out vec4 outColor;

void main()
{
  // NOTE: samplerCubes assume y is up, so we must adjust accordingly
  vec3 yIsUpTexCoord = vec3(inTexCoord.x, inTexCoord.z, -inTexCoord.y);
  outColor = texture(skyboxTex, yIsUpTexCoord);
}