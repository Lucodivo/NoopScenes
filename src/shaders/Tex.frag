#version 420
layout (location = 0) in vec2 inTexCoord;

uniform sampler2D texture;

layout (location = 0) out vec4 outColor;

void main()
{
  vec4 texVals = texture(tex, inTexCoord);
  if(texVals.a < 0.05) discard;
  outColor = texVals;
}