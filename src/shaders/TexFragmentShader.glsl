#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler2D texture;

void main()
{
  vec4 texVals = texture(tex, TextureCoord);
  if(texVals.a < 0.05) discard;
  FragColor = texVals;
}