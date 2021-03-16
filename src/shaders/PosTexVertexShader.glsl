#version 410
layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout (location = 0) out vec2 outTexCoord;

void main()
{
  gl_Position = projection * view * model * vec4(inPos, 1.0);
  outTexCoord = inTexCoord;
}