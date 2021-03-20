#version 420

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inTexCoord;

layout (binding = 1, std140) uniform LightInfoUBO {
  vec3 directionalLightColor;
  vec3 ambientLightColor;
  vec3 directionalLightDirToSource;
} ubo;

layout (location = 0) out vec4 outColor;

void main() {
  float directLightContribution = max(dot(inNormal, ubo.directionalLightDirToSource), 0.0);
  vec4 resultColor = vec4(ubo.ambientLightColor + (ubo.directionalLightColor * directLightContribution), 1.0);
  outColor = resultColor;
}
