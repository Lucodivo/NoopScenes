#pragma once

struct VertexAtt {
  u32 arrayObject;
  u32 bufferObject;
  u32 indexObject;
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
const u32 cubePositionIndices[]{
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

VertexAtt initializeCubePositionVertexAttBuffers() {
  VertexAtt vertexAtt;
  glGenVertexArrays(1, &vertexAtt.arrayObject); // vertex array object
  glGenBuffers(1, &vertexAtt.bufferObject); // vertex buffer object backing the VAO
  glGenBuffers(1, &vertexAtt.indexObject);

  glBindVertexArray(vertexAtt.arrayObject);
  glBindBuffer(GL_ARRAY_BUFFER, vertexAtt.bufferObject);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositionAttributes), cubePositionAttributes, GL_STATIC_DRAW);

  // set the vertex attributes
  // position attribute
  glVertexAttribPointer(0, // index
                        3, // size
                        GL_FLOAT, // type of data
                        GL_FALSE, // whether the data needs to be normalized
                        cubePositionSizeInBytes, // stride
                        (void*)0); // offset
  glEnableVertexAttribArray(0);

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