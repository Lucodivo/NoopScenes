#version 420

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inFragmentWorldPos;
layout (location = 3) in vec3 inCameraWorldPos;

layout (binding = 1, std140) uniform LightInfoUBO {
  vec3 directionalLightColor;
  vec3 ambientLightColor;
  vec3 directionalLightDirToSource;
} ubo;

uniform sampler2D albedoTex;
uniform sampler2D normalTex;

layout (location = 0) out vec4 outColor;

vec3 getNormal();

void main() {
  vec3 albedoColor = texture(albedoTex, inTexCoord).rgb;

  vec3 surfaceNormal = getNormal();
  float directLightContribution = max(dot(surfaceNormal, ubo.directionalLightDirToSource), 0.0);
  vec3 lightContribution = vec3(ubo.ambientLightColor + (ubo.directionalLightColor * directLightContribution));
  outColor = vec4(lightContribution * albedoColor, 1.0);
}

// Note: https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr.vert
vec3 getNormal()
{
  // Perturb normal, see http://www.thetenthplanet.de/archives/1180
  vec3 tangentNormal = texture(normalTex, inTexCoord).xyz * 2.0 - 1.0;

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