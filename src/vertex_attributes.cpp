// TODO: When, if ever, does this vertex attribute data get deleted?
global_variable union {
  struct {
    VertexAtt cubePos;
    VertexAtt invertedCubePos;
    VertexAtt cubePos_OpenNegYFace;
    VertexAtt invertedCubePos_OpenNegYFace;
    VertexAtt quadPos;
    VertexAtt quadPosTex;
  };
  VertexAtt array[6];
} globalVertexAtts;

#define GLOBAL_VERTEX_ATT_COUNT

const u32 cubePosAttStrideInBytes = 3 * sizeof(f32);
const f32 cubePosAtts[] = {
        // positions
        // face #1 (negative x)
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        // face #2 (positive x)
        0.5f,  0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        // face #3 (negative y)
        -0.5f, -0.5f, -0.5f,
        0.5f,  -0.5f,  -0.5f,
        0.5f,  -0.5f,   0.5f,
        -0.5f, -0.5f,  0.5f,
        // face #4 (positive y)
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        // face #5 (negative z)
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        // face #6 (positive z)
        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
};
const u8 cubePosAttsIndices[]{
        // face #1 (negative x)
        0,  1,  2,
        2,  3,  0,
        // face #2 (positive x)
        4,  5,  6,
        6,  7,  4,
        // face #3 (negative y)
        8,  9, 10,
        10, 11,  8,
        // face #4 (positive y)
        12, 13, 14,
        14, 15, 12,
        // face #5 (negative z)
        16, 17, 18,
        18, 19, 16,
        // face #6 (positive z)
        20, 21, 22,
        22, 23, 20,
};
const u8 cubePosAttsIndices_OpenNegYFace[]{
        // face #1 (negative x)
        0,  2,  1,
        2,  0,  3,
        // face #2 (positive x)
        4,  6,  5,
        6,  4,  7,
        // face #4 (positive y)
        12, 14,13,
        14, 12,15,
        // face #5 (negative z)
        16, 18,17,
        18, 16,19,
        // face #6 (positive z)
        20, 22,21,
        22, 20,23,
};
const u8 invertedWindingCubePosAttsIndices[]{
        // face #1 (negative x)
        0,  2,  1,
        2,  0,  3,
        // face #2 (positive x)
        4,  6,  5,
        6,  4,  7,
        // face #3 (negative y)
        8,  10, 9,
        10, 8, 11,
        // face #4 (positive y)
        12, 14,13,
        14, 12,15,
        // face #5 (negative z)
        16, 18,17,
        18, 16,19,
        // face #6 (positive z)
        20, 22,21,
        22, 20,23,
};
const u8 invertedWindingCubePosAttsIndices_OpenNegYFace[]{
        // face #1 (negative x)
        0,  2,  1,
        2,  0,  3,
        // face #2 (positive x)
        4,  6,  5,
        6,  4,  7,
        // face #4 (positive y)
        12, 14,13,
        14, 12,15,
        // face #5 (negative z)
        16, 18,17,
        18, 16,19,
        // face #6 (positive z)
        20, 22,21,
        22, 20,23,
};
const u32 cubeFaceNegativeXIndicesOffset = 0;
const u32 cubeFacePositiveXIndicesOffset = 6;
const u32 cubeFaceNegativeYIndicesOffset = 12;
const u32 cubeFacePositiveYIndicesOffset = 18;
const u32 cubeFaceNegativeZIndicesOffset = 24;
const u32 cubeFacePositiveZIndicesOffset = 30;

// ===== Quad values (vec3 position, vec2 tex) =====
// faces -Y direction
const f32 quadPosVertexAttributes[] = {
        // positions
        -0.5f,  0.0f, -0.5f,
         0.5f,  0.0f, -0.5f,
         0.5f,  0.0f,  0.5f,
        -0.5f,  0.0f,  0.5f,
};
const u32 quadPosVertexAttSizeInBytes = 3 * sizeof(f32);
const f32 quadPosTexVertexAttributes[] = {
        // positions        // texCoords
        -0.5f,  0.0f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.0f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.0f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.0f,  0.5f,  0.0f, 1.0f,
};
const u32 quadPosTexVertexAttSizeInBytes = 5 * sizeof(f32);
const vec3 quadVertexAttNormal = {0.0f, -1.0f, 0.0f};
const u8 quadIndices[]{
        0, 1, 2,
        0, 2, 3,
};

mat4 quadModelMatrix(const vec3& centerPos, const vec3& desiredNormal, const f32 width, const f32 height) {
  mat4 scaleMat = scale_mat4(vec3{width, 1.0f, height});;
  mat4 rotationMat = rotate_mat4(orient(quadVertexAttNormal, desiredNormal));
  mat4 translationMat = translate_mat4(centerPos);
  return translationMat * rotationMat * scaleMat;
}

u32 convertSizeInBytesToOpenGLUIntType(u8 sizeInBytes) {
  switch(sizeInBytes) {
    case 1:
      return GL_UNSIGNED_BYTE;
    case 2:
      return GL_UNSIGNED_SHORT;
    case 4:
      return GL_UNSIGNED_INT;
  }

  Assert(false); // Note: if not 1, 2, or 4 throw error
  return 0;
}

void initCubeVertexAttBuffers()
{
  if (globalVertexAtts.cubePos.indexCount == 0)
  { // uninitialized
    // normal cube vertex attribute
    globalVertexAtts.cubePos.indexCount = ArrayCount(cubePosAttsIndices);
    globalVertexAtts.cubePos.indexTypeSizeInBytes = sizeof(cubePosAttsIndices) / globalVertexAtts.cubePos.indexCount;

    const u32 positionAttributeIndex = 0;

    glGenVertexArrays(1, &globalVertexAtts.cubePos.arrayObject); // vertex array object
    glGenBuffers(1, &globalVertexAtts.cubePos.bufferObject); // vertex buffer object backing the VAO
    glGenBuffers(1, &globalVertexAtts.cubePos.indexObject);

    glBindVertexArray(globalVertexAtts.cubePos.arrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, globalVertexAtts.cubePos.bufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubePosAtts), cubePosAtts, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(positionAttributeIndex, // index
                          3, // size
                          GL_FLOAT, // type of data
                          GL_FALSE, // whether the data needs to be normalized
                          cubePosAttStrideInBytes, // stride
                          (void*) 0); // offset
    glEnableVertexAttribArray(positionAttributeIndex);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.cubePos.indexObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.cubePos.indexCount, cubePosAttsIndices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Must unbind EBO AFTER unbinding VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // open cube vertex attribute
    globalVertexAtts.cubePos_OpenNegYFace = globalVertexAtts.cubePos;
    globalVertexAtts.cubePos_OpenNegYFace.indexCount = ArrayCount(cubePosAttsIndices_OpenNegYFace);
    glGenVertexArrays(1, &globalVertexAtts.cubePos_OpenNegYFace.arrayObject); // vertex array object
    glGenBuffers(1, &globalVertexAtts.cubePos_OpenNegYFace.indexObject);
    glBindVertexArray(globalVertexAtts.cubePos_OpenNegYFace.arrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, globalVertexAtts.cubePos_OpenNegYFace.bufferObject);

    // position attribute
    glVertexAttribPointer(positionAttributeIndex, // index
                          3, // size
                          GL_FLOAT, // type of data
                          GL_FALSE, // whether the data needs to be normalized
                          cubePosAttStrideInBytes, // stride
                          (void*) 0); // offset
    glEnableVertexAttribArray(positionAttributeIndex);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.cubePos_OpenNegYFace.indexObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.cubePos_OpenNegYFace.indexCount, cubePosAttsIndices_OpenNegYFace,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Must unbind EBO AFTER unbinding VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // inverted cube vertex att
    globalVertexAtts.invertedCubePos = globalVertexAtts.cubePos;
    glGenVertexArrays(1, &globalVertexAtts.invertedCubePos.arrayObject); // vertex array object
    glGenBuffers(1, &globalVertexAtts.invertedCubePos.indexObject);
    glBindVertexArray(globalVertexAtts.invertedCubePos.arrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, globalVertexAtts.invertedCubePos.bufferObject);

    // position attribute
    glVertexAttribPointer(positionAttributeIndex, // index
                          3, // size
                          GL_FLOAT, // type of data
                          GL_FALSE, // whether the data needs to be normalized
                          cubePosAttStrideInBytes, // stride
                          (void*) 0); // offset
    glEnableVertexAttribArray(positionAttributeIndex);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.invertedCubePos.indexObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.invertedCubePos.indexCount, invertedWindingCubePosAttsIndices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Must unbind EBO AFTER unbinding VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // open cube vertex attribute
    globalVertexAtts.invertedCubePos_OpenNegYFace = globalVertexAtts.cubePos_OpenNegYFace;
    globalVertexAtts.invertedCubePos_OpenNegYFace.indexCount = ArrayCount(cubePosAttsIndices_OpenNegYFace);
    glGenVertexArrays(1, &globalVertexAtts.invertedCubePos_OpenNegYFace.arrayObject); // vertex array object
    glGenBuffers(1, &globalVertexAtts.invertedCubePos_OpenNegYFace.indexObject);
    glBindVertexArray(globalVertexAtts.invertedCubePos_OpenNegYFace.arrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, globalVertexAtts.invertedCubePos_OpenNegYFace.bufferObject);

    // position attribute
    glVertexAttribPointer(positionAttributeIndex, // index
                          3, // size
                          GL_FLOAT, // type of data
                          GL_FALSE, // whether the data needs to be normalized
                          cubePosAttStrideInBytes, // stride
                          (void*) 0); // offset
    glEnableVertexAttribArray(positionAttributeIndex);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.invertedCubePos_OpenNegYFace.indexObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.invertedCubePos_OpenNegYFace.indexCount, invertedWindingCubePosAttsIndices_OpenNegYFace,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Must unbind EBO AFTER unbinding VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

void initQuadVertexAttBuffers() {
  if (globalVertexAtts.quadPosTex.indexCount == 0)
  { // uninitialized
    globalVertexAtts.quadPosTex.indexCount = ArrayCount(quadIndices);
    globalVertexAtts.quadPosTex.indexTypeSizeInBytes = sizeof(quadIndices) / globalVertexAtts.quadPosTex.indexCount;
    const u32 positionAttributeIndex = 0;
    const u32 textureAttributeIndex = 1;

    glGenVertexArrays(1, &globalVertexAtts.quadPosTex.arrayObject);
    glGenBuffers(1, &globalVertexAtts.quadPosTex.bufferObject);
    glGenBuffers(1, &globalVertexAtts.quadPosTex.indexObject);

    glBindVertexArray(globalVertexAtts.quadPosTex.arrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, globalVertexAtts.quadPosTex.bufferObject);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(quadPosTexVertexAttributes),
                 quadPosTexVertexAttributes,
                 GL_STATIC_DRAW);

    // set the vertex attributes (position and texture)
    // position attribute
    glVertexAttribPointer(positionAttributeIndex,
                          3, // attribute size
                          GL_FLOAT,
                          GL_FALSE,
                          quadPosTexVertexAttSizeInBytes,
                          (void*) 0);
    glEnableVertexAttribArray(positionAttributeIndex);

    // texture attribute
    glVertexAttribPointer(textureAttributeIndex,
                          2, // attribute size
                          GL_FLOAT,
                          GL_FALSE,
                          quadPosTexVertexAttSizeInBytes,
                          (void*) (3 * sizeof(f32)));
    glEnableVertexAttribArray(textureAttributeIndex);

    // bind element buffer object to give indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.quadPosTex.indexObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // unbind VBO, VAO, & EBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Must unbind EBO AFTER unbinding VAO, since VAO stores all glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _) calls
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    globalVertexAtts.quadPos = globalVertexAtts.quadPosTex;

    // only need to create a new array object, the index buffer and data buffer will be the same
    glGenVertexArrays(1, &globalVertexAtts.quadPos.arrayObject);
    glBindVertexArray(globalVertexAtts.quadPos.arrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, globalVertexAtts.quadPos.bufferObject);

    // set the vertex attributes (position and texture)
    // position attribute
    glVertexAttribPointer(positionAttributeIndex,
                          3, // attribute size
                          GL_FLOAT,
                          GL_FALSE,
                          quadPosTexVertexAttSizeInBytes,
                          (void*) 0);
    glEnableVertexAttribArray(positionAttributeIndex);

    // bind element buffer object to give indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, globalVertexAtts.quadPos.indexObject);

    // unbind VBO, VAO, & EBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Must unbind EBO AFTER unbinding VAO, since VAO stores all glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _) calls
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

void initGlobalVertexAtts() {
  initCubeVertexAttBuffers();
  initQuadVertexAttBuffers();
}

VertexAtt cubePosVertexAttBuffers(bool invertedWindingOrder, bool openNegYFace) {
  initGlobalVertexAtts();
  return invertedWindingOrder ?
         (openNegYFace ? globalVertexAtts.invertedCubePos_OpenNegYFace : globalVertexAtts.invertedCubePos) :
         (openNegYFace ? globalVertexAtts.cubePos_OpenNegYFace : globalVertexAtts.cubePos);
}

VertexAtt quadPosVertexAttBuffers(b32 textureAtt) {
  initGlobalVertexAtts();
  return textureAtt ? globalVertexAtts.quadPosTex : globalVertexAtts.quadPos;
}

internal_func void drawIndexedTriangles(const VertexAtt* vertexAtt, u32 indexCount, u64 indexOffset)
{
  glBindVertexArray(vertexAtt->arrayObject); // NOTE: Binding every time is unnecessary if the same vertexAtt is used for multiple calls in a row
  glDrawElements(GL_TRIANGLES, // drawing mode
                 indexCount, // number of elements
                 convertSizeInBytesToOpenGLUIntType(vertexAtt->indexTypeSizeInBytes), // type of the indices
                 (void*)(indexOffset * vertexAtt->indexTypeSizeInBytes)); // offset in the EBO
}

void drawTriangles(const VertexAtt* vertexAtt, u32 count, u32 offset)
{
  Assert(vertexAtt->indexCount >= (offset + count));
  drawIndexedTriangles(vertexAtt, count, offset);
}

void drawTriangles(const VertexAtt* vertexAtt)
{
  drawTriangles(vertexAtt, vertexAtt->indexCount, 0);
}

void deleteVertexAtt(VertexAtt* vertexAtt)
{
  // TODO: prevent from deleting global vertex atts
  glDeleteBuffers(1, &vertexAtt->indexObject);
  glDeleteVertexArrays(1, &vertexAtt->arrayObject);
  glDeleteBuffers(1, &vertexAtt->bufferObject);
}

void deleteVertexAtts(VertexAtt** vertexAtts, u32 count)
{
  // TODO: prevent from deleting global vertex atts

  u32* deleteBufferObjects = new u32[count * 3];
  u32* deleteIndexBufferObjects = deleteBufferObjects + count;
  u32* deleteVertexArrays = deleteIndexBufferObjects + count;
  for(u32 i = 0; i < count; i++) {
    VertexAtt vertexAtt = *vertexAtts[i];
    deleteBufferObjects[i] = vertexAtt.bufferObject;
    deleteIndexBufferObjects[i] = vertexAtt.indexObject;
    deleteVertexArrays[i] = vertexAtt.arrayObject;
  }

  glDeleteBuffers(count * 2, deleteBufferObjects);
  glDeleteVertexArrays(count, deleteVertexArrays);

  delete[] deleteBufferObjects;
}
