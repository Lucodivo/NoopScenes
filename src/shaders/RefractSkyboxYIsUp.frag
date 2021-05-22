#version 420 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inCameraPos;

#define REFRACTIVE_INDEX_MIN 1.33f // Water
#define REFRACTIVE_INDEX_MAX 2.42f // diamond

uniform samplerCube skyboxTex;


layout (location = 0) out vec4 outColor;

void main()
{
  float refractionRatio = 1.0 / REFRACTIVE_INDEX_MIN;
  vec3 cameraToPos = normalize(inPos - inCameraPos);
  vec3 reflected = refract(cameraToPos, inNormal, refractionRatio);
  vec3 reflectedYIsUp = vec3(reflected.x, reflected.z, -reflected.y);
  outColor = vec4(texture(skyboxTex, reflectedYIsUp).rgb, 1.0);
}