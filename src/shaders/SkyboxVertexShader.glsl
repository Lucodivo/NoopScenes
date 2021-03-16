#version 410
layout (location = 0) in vec3 inPos;

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) out vec3 outTexCoord;

void main()
{
  outTexCoord = inPos;
  vec4 pos = projection * view * vec4(inPos, 1.0);
  gl_Position = pos.xyww;
}  