#pragma once

u32 projectionViewModelUBOBindingIndex = 0;
struct ProjectionViewModelUBO {  // base alignment   // aligned offset
  mat4 projection;               // 4                // 0
  mat4 view;                     // 4                // 64
  mat4 model;                    // 4                // 128
};

struct LightUBO {
  vec3 directionalLightColor;
  u8 __padding1;
  vec3 ambientLightColor;
  u8 __padding2;
  vec3 directionalLightDirToSource;
  u8 __padding3;
};

struct FragUBO {
  f32 time;
};