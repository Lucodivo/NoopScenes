#version 420
layout (location = 0) in vec3 inPos;

layout (binding = 0, std140) uniform UBO { // base alignment   // aligned offset
  mat4 projection;                         // 64               // 0
  mat4 view;                               // 64               // 64
//mat4 model; //TODO: Available but not needed
} ubo;

layout (location = 0) out vec3 outTexCoord;

void main()
{
  outTexCoord = inPos;
  mat4 skyboxViewMat = mat4(mat3(ubo.view)); // remove translation
  vec4 pos = ubo.projection * skyboxViewMat * vec4(inPos, 1.0);
  // NOTE: z = w effectively sets depth to furthest away
  gl_Position = pos.xyww;
}  