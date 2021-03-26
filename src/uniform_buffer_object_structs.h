#pragma once

u32 projectionViewModelUBOBindingIndex = 0;
struct ProjectionViewModelUBO {  // base alignment   // aligned offset
  Mat4 projection;               // 4                // 0
  Mat4 view;                     // 4                // 64
  Mat4 model;                    // 4                // 128
};