#pragma once

u32 projectionViewModelUBOBindingIndex = 0;
struct ProjectionViewModelUBO {  // base alignment   // aligned offset
  glm::mat4 projection;          // 4                // 0
  glm::mat4 view;                // 4                // 64
  glm::mat4 model;               // 4                // 128
};