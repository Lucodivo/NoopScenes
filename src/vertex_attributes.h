#pragma once

struct VertexAtt {
  u32 arrayObject;
  u32 bufferObject;
  u32 indexObject;
};

const u32 cubePositionSizeInBytes = 3 * sizeof(f32);
const f32 cubePositionAttributes[] = {
        // positions
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
};

const u32 cubePositionIndices[]{
        0, 2, 1,
        1, 2, 3,
        0, 1, 4,
        1, 5, 4,
        0, 4, 2,
        4, 6, 2,
        2, 6, 3,
        6, 7, 3,
        1, 7, 5,
        1, 3, 7,
        4, 5, 7,
        4, 7, 6,
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