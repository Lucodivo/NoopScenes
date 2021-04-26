#pragma once

#define VERTEX_ATT_NO_INDEX_OBJECT -1

struct VertexAtt {
  GLuint arrayObject;
  GLuint bufferObject;
  GLuint indexObject;
  u32 indexCount;
  u32 indexTypeSizeInBytes;
};

enum CubeSide {
  CubeSide_NegativeX,
  CubeSide_PositiveX,
  CubeSide_NegativeY,
  CubeSide_PositiveY,
  CubeSide_NegativeZ,
  CubeSide_PositiveZ
};

const vec3 cubeFaceNegativeXCenter{-0.5f, 0.0f, 0.0f};
const vec3 cubeFacePositiveXCenter{0.5f, 0.0f, 0.0f};
const vec3 cubeFaceNegativeYCenter{0.0f, -0.5f, 0.0f};
const vec3 cubeFacePositiveYCenter{0.0f, 0.5f, 0.0f};
const vec3 cubeFaceNegativeZCenter{0.0f, 0.0f, -0.5f};
const vec3 cubeFacePositiveZCenter{0.0f, 0.0f, 0.5f};
// center values are simply doubled to get a normalized vector
const vec3 cubeFaceNegativeXNormal = cubeFaceNegativeXCenter * 2.0f;
const vec3 cubeFacePositiveXNormal = cubeFacePositiveXCenter * 2.0f;
const vec3 cubeFaceNegativeZNormal = cubeFaceNegativeZCenter * 2.0f;
const vec3 cubeFacePositiveZNormal = cubeFacePositiveZCenter * 2.0f;
const vec3 cubeFaceNegativeYNormal = cubeFaceNegativeYCenter * 2.0f;
const vec3 cubeFacePositiveYNormal = cubeFacePositiveYCenter * 2.0f;
const BoundingBox cubeVertAttBoundingBox = {
        {-0.5, -0.5, -0.5},
        {1.0f, 1.0f, 1.0f}
};

// TODO: move to cpp
const u32 cubeFaceNegativeXIndicesOffset = 0;
const u32 cubeFacePositiveXIndicesOffset = 6;
const u32 cubeFaceNegativeYIndicesOffset = 12;
const u32 cubeFacePositiveYIndicesOffset = 18;
const u32 cubeFaceNegativeZIndicesOffset = 24;
const u32 cubeFacePositiveZIndicesOffset = 30;


// ===== Quad values (vec2 position, vec2 tex) =====

VertexAtt cubePositionVertexAttBuffers(bool invertedWindingOrder = false);
VertexAtt quadPosTexVertexAttBuffers(CubeSide cubeSide, bool invertedWindingOrder = false);

void drawTriangles(const VertexAtt* vertexAtt, u32 count, u32 offset);
void drawTriangles(const VertexAtt* vertexAtt);

void deleteVertexAtt(VertexAtt* vertexAtt);
void deleteVertexAtts(VertexAtt** vertexAtts, u32 count);