#pragma once

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

VertexAtt initializeModelVertexBuffer(tinygltf::Model* model)
{
  struct gltfAttributeMetadata {
    u32 accessorIndex;
    u32 numComponents;
    u32 bufferViewIndex;
    u32 bufferIndex;
    u64 bufferByteOffset;
    u64 bufferByteLength;
  };

  const char* positionIndexKeyString = "POSITION";
  const char* normalIndexKeyString = "NORMAL";
  const char* texture0IndexKeyString = "TEXCOORD_0";

  // TODO: Pull vertex  attributes for more then the first primitive of the first mesh of a model
  Assert(!model->meshes.empty());
  Assert(!model->meshes[0].primitives.empty());
  tinygltf::Primitive primitive = model->meshes[0].primitives[0];
  Assert(primitive.indices > -1); // TODO: Should we deal with models that don't have indices?
  std::vector<tinygltf::Accessor>* accessors = &model->accessors;
  std::vector<tinygltf::BufferView>* bufferViews = &model->bufferViews;

  auto populateAttributeMetadata = [primitive, accessors, bufferViews](const char* keyString) -> gltfAttributeMetadata {
    gltfAttributeMetadata result;
    result.accessorIndex = primitive.attributes.at(keyString);
    result.numComponents = tinygltf::GetNumComponentsInType(accessors->at(result.accessorIndex).type);
    result.bufferViewIndex = accessors->at(result.accessorIndex).bufferView;
    result.bufferIndex = bufferViews->at(result.bufferViewIndex).buffer;
    result.bufferByteOffset = bufferViews->at(result.bufferViewIndex).byteOffset;
    result.bufferByteLength = bufferViews->at(result.bufferViewIndex).byteLength;
    return result;
  };

  // TODO: Allow variability in attributes beyond POSITION, NORMAL, TEXCOORD_0?
  Assert(primitive.attributes.find(positionIndexKeyString) != primitive.attributes.end());
  gltfAttributeMetadata positionAttribute = populateAttributeMetadata(positionIndexKeyString);

  b32 normalAttributesAvailable = primitive.attributes.find(normalIndexKeyString) != primitive.attributes.end();
  gltfAttributeMetadata normalAttribute;
  if(normalAttributesAvailable) { // normal attribute data
    normalAttribute = populateAttributeMetadata(normalIndexKeyString);
    Assert(positionAttribute.bufferIndex == normalAttribute.bufferIndex);
  }

  b32 texture0AttributesAvailable = primitive.attributes.find(texture0IndexKeyString) != primitive.attributes.end();
  gltfAttributeMetadata texture0Attribute;
  if(texture0AttributesAvailable) { // texture 0 uv coord attribute data
    texture0Attribute = populateAttributeMetadata(texture0IndexKeyString);
    Assert(positionAttribute.bufferIndex == texture0Attribute.bufferIndex);
  }

  // TODO: Handle vertex attributes that don't share the same buffer?
  u32 vertexAttBufferIndex = positionAttribute.bufferIndex;
  Assert(model->buffers.size() > vertexAttBufferIndex);

  u32 indicesAccessorIndex = primitive.indices;
  u32 indicesGLTFBufferViewIndex = accessors->at(indicesAccessorIndex).bufferView;
  u32 indicesGLTFBufferIndex = bufferViews->at(indicesGLTFBufferViewIndex).buffer;
  u64 indicesGLTFBufferByteOffset = bufferViews->at(indicesGLTFBufferViewIndex).byteOffset;
  u64 indicesGLTFBufferByteLength = bufferViews->at(indicesGLTFBufferViewIndex).byteLength;

  u8* dataOffset = model->buffers[indicesGLTFBufferIndex].data.data() + indicesGLTFBufferByteOffset;

  VertexAtt vertexAtt;
  vertexAtt.indexCount = u32(accessors->at(indicesAccessorIndex).count);
  vertexAtt.indexTypeSizeInBytes = tinygltf::GetComponentSizeInBytes(accessors->at(indicesAccessorIndex).componentType);
  u64 sizeOfAttributeData = texture0Attribute.bufferByteOffset + texture0Attribute.bufferByteLength;
  Assert(model->buffers[vertexAttBufferIndex].data.size() >= sizeOfAttributeData);
  const u32 positionAttributeIndex = 0;
  const u32 normalAttributeIndex = 1;
  const u32 texture0AttributeIndex = 2;

  glGenVertexArrays(1, &vertexAtt.arrayObject);
  glGenBuffers(1, &vertexAtt.bufferObject);
  glGenBuffers(1, &vertexAtt.indexObject);

  glBindVertexArray(vertexAtt.arrayObject);
  glBindBuffer(GL_ARRAY_BUFFER, vertexAtt.bufferObject);
  glBufferData(GL_ARRAY_BUFFER,
               sizeOfAttributeData,
               model->buffers[vertexAttBufferIndex].data.data(),
               GL_STATIC_DRAW);

  // set the vertex attributes (position and texture)
  // position attribute
  glVertexAttribPointer(positionAttributeIndex,
                        positionAttribute.numComponents, // attribute size
                        GL_FLOAT, // type of data
                        GL_FALSE, // should data be normalized
                        positionAttribute.numComponents * sizeof(f32),// stride
                        (void*)positionAttribute.bufferByteOffset); // offset of first component
  glEnableVertexAttribArray(positionAttributeIndex);

  // normal attribute
  if(normalAttributesAvailable) {
    glVertexAttribPointer(normalAttributeIndex,
                          normalAttribute.numComponents, // attribute size
                          GL_FLOAT,
                          GL_FALSE,
                          normalAttribute.numComponents * sizeof(f32),
                          (void*)normalAttribute.bufferByteOffset);
    glEnableVertexAttribArray(normalAttributeIndex);
  }

  // texture 0 UV Coord attribute
  if(texture0AttributesAvailable) {
    glVertexAttribPointer(texture0AttributeIndex,
                          texture0Attribute.numComponents, // attribute size
                          GL_FLOAT,
                          GL_FALSE,
                          texture0Attribute.numComponents * sizeof(f32),
                          (void*)texture0Attribute.bufferByteOffset);
    glEnableVertexAttribArray(texture0AttributeIndex);
  }

  // bind element buffer object to give indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexAtt.indexObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesGLTFBufferByteLength, dataOffset, GL_STATIC_DRAW);

  // unbind VBO & VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  return vertexAtt;
}

void loadModelsVertexAtt(const char** filePaths, VertexAtt** returnVertAtts, u32 modelCount) {
  tinygltf::TinyGLTF loader;
  std::vector<tinygltf::Model> models;
  models.reserve(modelCount);

  for(u32 i = 0; i < modelCount; ++i) {
    std::string err;
    std::string warn;
    tinygltf::Model model;

    //bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath); // for .gltf
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePaths[i]); // for binary glTF(.glb)

    if (!warn.empty()) {
      printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
      printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
      printf("Failed to parse glTF\n");
    }

    models.push_back(model);
  }

  for(u32 i = 0; i < modelCount; ++i) {
    *(returnVertAtts[i]) = initializeModelVertexBuffer(&models[i]);
  }
}