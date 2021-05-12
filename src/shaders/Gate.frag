#version 420

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inFragmentWorldPos;
layout (location = 3) in vec3 inCameraWorldPos;

layout (binding = 1, std140) uniform FragUBO {
  float time;
} fragUbo;

layout (binding = 2, std140) uniform LightInfoUBO {
  vec3 directionalLightColor;
  vec3 ambientLightColor;
  vec3 directionalLightDirToSource;
} lightInfoUbo;

uniform sampler2D albedoTex;
uniform sampler2D normalTex;
uniform sampler2D tiledNoiseTex;

layout (location = 0) out vec4 outColor;

vec3 getNormal(vec2 texCoord);

void main() {
  vec2 time = vec2(fragUbo.time * 5.0);
  vec2 albedoTexSize = textureSize(albedoTex, 0);
  vec2 tiledNoiseTexSize = textureSize(tiledNoiseTex, 0);

  vec2 texCoordTime = inTexCoord + (time / albedoTexSize);

  vec2 noiseTexCoord = (inTexCoord * albedoTexSize) / tiledNoiseTexSize;
  float noise = texture(tiledNoiseTex, inTexCoord + (vec2(-time.x, time.y) / tiledNoiseTexSize)).r;
  noise = noise - 0.5;
  vec2 texCoordTimeNoise = inTexCoord + (vec2(noise) * tiledNoiseTexSize / albedoTexSize) * 0.005;

  vec3 albedoColor = texture(albedoTex, texCoordTimeNoise).rgb;

  vec3 surfaceNormal = getNormal(texCoordTimeNoise);
  float directLightContribution = max(dot(surfaceNormal, lightInfoUbo.directionalLightDirToSource), 0.0);
  vec3 lightContribution = vec3(lightInfoUbo.ambientLightColor + (lightInfoUbo.directionalLightColor * directLightContribution));
  outColor = vec4(lightContribution * albedoColor, 1.0);
}

// Note: https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr.vert
vec3 getNormal(vec2 texCoord)
{
  // Perturb normal, see http://www.thetenthplanet.de/archives/1180
  vec3 tangentNormal = texture(normalTex, texCoord).xyz * 2.0 - 1.0;

  vec3 q1 = dFdx(inFragmentWorldPos);
  vec3 q2 = dFdy(inFragmentWorldPos);
  vec2 st1 = dFdx(inTexCoord);
  vec2 st2 = dFdy(inTexCoord);

  vec3 N = normalize(inNormal);
  vec3 T = normalize(q1 * st2.t - q2 * st1.t);
  vec3 B = -normalize(cross(N, T));
  mat3 TBN = mat3(T, B, N);

  return normalize(TBN * tangentNormal);
}