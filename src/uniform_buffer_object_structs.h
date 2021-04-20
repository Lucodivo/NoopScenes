#pragma once

u32 projectionViewModelUBOBindingIndex = 0;
struct ProjectionViewModelUBO {  // base alignment   // aligned offset
  mat4 projection;               // 4                // 0
  mat4 view;                     // 4                // 64
  mat4 model;                    // 4                // 128
};