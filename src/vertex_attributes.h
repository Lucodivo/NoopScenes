#pragma once

#define VERTEX_ATT_NO_INDEX_OBJECT -1

struct VertexAtt {
  u32 arrayObject;
  u32 bufferObject;
  u32 indexObject;
  u32 indexCount;
  u32 indexTypeSizeInBytes;
};

const u32 cubePositionSizeInBytes = 3 * sizeof(f32);
const f32 cubePositionAttributes[] = {
        // positions
        // face #1
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        // face #2
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        // face #3
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        // face #4
        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // face #5
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        // face #6
        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
};
const u8 cubePositionIndices[]{
        0, 1, 2, // 0
        2, 3, 0,
        4, 6, 5, // 1
        6, 4, 7,
        8, 9, 10, // 2
        10, 11, 8,
        12, 14, 13, // 3
        14, 12, 15,
        16, 18, 17, // 4
        18, 16, 19,
        20, 21, 22, // 5
        22, 23, 20,
};


// ===== Quad values (vec3 position, vec2 tex) =====
const u32 quadPosTexVertexAttSizeInBytes = 5 * sizeof(f32);
const f32 quadPosTexVertexAttributes[] = {
        // positions        // texCoords
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.0f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.0f,  1.0f, 1.0f,
};
const u8 quadIndices[]{
        0, 1, 2,
        0, 2, 3,
};
// ===== Quad values (vec2 position, vec2 tex) =====

u32 convertSizeInBytesToOpenGLUIntType(u8 sizeInBytes) {
  Assert(sizeInBytes > 0);
  Assert(sizeInBytes != 3);
  Assert(sizeInBytes <= 4);

  switch(sizeInBytes) {
    case 1:
      return GL_UNSIGNED_BYTE;
    case 2:
      return GL_UNSIGNED_SHORT;
    case 3:
      return GL_UNSIGNED_INT;
  }
}

VertexAtt initializeCubePositionVertexAttBuffers() {
  VertexAtt vertexAtt;
  vertexAtt.indexCount = ArrayCount(cubePositionIndices);
  vertexAtt.indexTypeSizeInBytes = sizeof(cubePositionIndices) / vertexAtt.indexCount;
  const u32 positionAttributeIndex = 0;

  glGenVertexArrays(1, &vertexAtt.arrayObject); // vertex array object
  glGenBuffers(1, &vertexAtt.bufferObject); // vertex buffer object backing the VAO
  glGenBuffers(1, &vertexAtt.indexObject);

  glBindVertexArray(vertexAtt.arrayObject);
  glBindBuffer(GL_ARRAY_BUFFER, vertexAtt.bufferObject);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositionAttributes), cubePositionAttributes, GL_STATIC_DRAW);

  // set the vertex attributes
  // position attribute
  glVertexAttribPointer(positionAttributeIndex, // index
                        3, // size
                        GL_FLOAT, // type of data
                        GL_FALSE, // whether the data needs to be normalized
                        cubePositionSizeInBytes, // stride
                        (void*)0); // offset
  glEnableVertexAttribArray(positionAttributeIndex);

  // bind element buffer object to give indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexAtt.indexObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubePositionIndices), cubePositionIndices, GL_STATIC_DRAW);

  // unbind VBO, VAO, & EBO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  // Must unbind EBO AFTER unbinding VAO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  return vertexAtt;
}

VertexAtt initializeQuadPosTexVertexAttBuffers() {
  VertexAtt vertexAtt;
  vertexAtt.indexCount = ArrayCount(quadIndices);
  vertexAtt.indexTypeSizeInBytes = sizeof(quadIndices) / vertexAtt.indexCount;
  const u32 positionAttributeIndex = 0;
  const u32 textureAttributeIndex = 1;

  glGenVertexArrays(1, &vertexAtt.arrayObject);
  glGenBuffers(1, &vertexAtt.bufferObject);
  glGenBuffers(1, &vertexAtt.indexObject);

  glBindVertexArray(vertexAtt.arrayObject);

  glBindBuffer(GL_ARRAY_BUFFER, vertexAtt.bufferObject);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(quadPosTexVertexAttributes),
               quadPosTexVertexAttributes,
               GL_STATIC_DRAW);

  // set the vertex attributes (position and texture)
  // position attribute
  glVertexAttribPointer(positionAttributeIndex,
                        2, // attribute size
                        GL_FLOAT,
                        GL_FALSE,
                        quadPosTexVertexAttSizeInBytes,
                        (void*)0);
  glEnableVertexAttribArray(positionAttributeIndex);

  // texture attribute
  glVertexAttribPointer(textureAttributeIndex,
                        2, // attribute size
                        GL_FLOAT,
                        GL_FALSE,
                        quadPosTexVertexAttSizeInBytes,
                        (void*)(3 * sizeof(f32)));
  glEnableVertexAttribArray(textureAttributeIndex);

  // bind element buffer object to give indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexAtt.indexObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

  // unbind VBO, VAO, & EBO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  // Must unbind EBO AFTER unbinding VAO, since VAO stores all glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _) calls
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  return vertexAtt;
}

file_access void drawIndexedTriangles(VertexAtt vertexAtt, u32 indexCount, u32 indexOffset) {
  glBindVertexArray(vertexAtt.arrayObject); // NOTE: Binding every time is unnecessary if the same vertexAtt is used for multiple calls in a row
  glDrawElements(GL_TRIANGLES, // drawing mode
                 indexCount, // number of elements
                 convertSizeInBytesToOpenGLUIntType(vertexAtt.indexTypeSizeInBytes), // type of the indices
                 (void*)(indexOffset * vertexAtt.indexTypeSizeInBytes)); // offset in the EBO
}

void drawTriangles(VertexAtt vertexAtt, u32 count, u32 offset) {
  Assert(vertexAtt.indexCount >= (offset + count));
  drawIndexedTriangles(vertexAtt, count, offset);
}

void drawTriangles(VertexAtt vertexAtt) {
  drawTriangles(vertexAtt, vertexAtt.indexCount, 0);
}

void deleteVertexAtt(VertexAtt vertexAtt) {
  glDeleteBuffers(1, &vertexAtt.indexObject);
  glDeleteVertexArrays(1, &vertexAtt.arrayObject);
  glDeleteBuffers(1, &vertexAtt.bufferObject);
}

void deleteVertexAtts(u32 count, VertexAtt* vertexAtts)
{
  u32* deleteBufferObjects = new u32[count * 3];
  u32* deleteIndexObjects = deleteBufferObjects + count;
  u32* deleteVertexArrays = deleteIndexObjects + count;
  for(u32 i = 0; i < count; i++) {
    deleteBufferObjects[i] = vertexAtts[i].bufferObject;
    deleteIndexObjects[i] = vertexAtts[i].indexObject;
    deleteVertexArrays[i] = vertexAtts[i].arrayObject;
  }

  glDeleteBuffers(count * 2, deleteBufferObjects);
  glDeleteVertexArrays(count, deleteVertexArrays);

  delete[] deleteBufferObjects;
}