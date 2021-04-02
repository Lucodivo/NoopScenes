#pragma once

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

struct TextureData {
  GLuint albedoTextureId;
  GLuint normalTextureId;
};

struct Model {
  VertexAtt vertexAtt;
  TextureData textureData;
  BoundingBox boundingBox;
};

void initializeModelVertexData(tinygltf::Model* gltfModel, Model* model)
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

  // TODO: Pull vertex  attributes for more then the first primitive of the first mesh of a gltfModel
  Assert(!gltfModel->meshes.empty());
  Assert(!gltfModel->meshes[0].primitives.empty());
  tinygltf::Primitive primitive = gltfModel->meshes[0].primitives[0];
  Assert(primitive.indices > -1); // TODO: Should we deal with models that don't have indices?
  std::vector<tinygltf::Accessor>* accessors = &gltfModel->accessors;
  std::vector<tinygltf::BufferView>* bufferViews = &gltfModel->bufferViews;

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
  f64* minValues = gltfModel->accessors[positionAttribute.accessorIndex].minValues.data();
  f64* maxValues = gltfModel->accessors[positionAttribute.accessorIndex].maxValues.data();
  model->boundingBox.min = Vec3(minValues[0], minValues[1], minValues[2]);
  model->boundingBox.dimensionInMeters = Vec3(maxValues[0], maxValues[1], maxValues[2]) - model->boundingBox.min;

  b32 normalAttributesAvailable = primitive.attributes.find(normalIndexKeyString) != primitive.attributes.end();
  gltfAttributeMetadata normalAttribute{};
  if(normalAttributesAvailable) { // normal attribute data
    normalAttribute = populateAttributeMetadata(normalIndexKeyString);
    Assert(positionAttribute.bufferIndex == normalAttribute.bufferIndex);
  }

  b32 texture0AttributesAvailable = primitive.attributes.find(texture0IndexKeyString) != primitive.attributes.end();
  gltfAttributeMetadata texture0Attribute{};
  if(texture0AttributesAvailable) { // texture 0 uv coord attribute data
    texture0Attribute = populateAttributeMetadata(texture0IndexKeyString);
    Assert(positionAttribute.bufferIndex == texture0Attribute.bufferIndex);
  }

  // TODO: Handle vertex attributes that don't share the same buffer?
  u32 vertexAttBufferIndex = positionAttribute.bufferIndex;
  Assert(gltfModel->buffers.size() > vertexAttBufferIndex);

  u32 indicesAccessorIndex = primitive.indices;
  u32 indicesGLTFBufferViewIndex = accessors->at(indicesAccessorIndex).bufferView;
  u32 indicesGLTFBufferIndex = bufferViews->at(indicesGLTFBufferViewIndex).buffer;
  u64 indicesGLTFBufferByteOffset = bufferViews->at(indicesGLTFBufferViewIndex).byteOffset;
  u64 indicesGLTFBufferByteLength = bufferViews->at(indicesGLTFBufferViewIndex).byteLength;

  u8* dataOffset = gltfModel->buffers[indicesGLTFBufferIndex].data.data() + indicesGLTFBufferByteOffset;

  model->vertexAtt.indexCount = u32(accessors->at(indicesAccessorIndex).count);
  model->vertexAtt.indexTypeSizeInBytes = tinygltf::GetComponentSizeInBytes(accessors->at(indicesAccessorIndex).componentType);
  u64 sizeOfAttributeData = positionAttribute.bufferByteLength + normalAttribute.bufferByteLength + texture0Attribute.bufferByteLength;
  Assert(gltfModel->buffers[vertexAttBufferIndex].data.size() >= sizeOfAttributeData);
  const u32 positionAttributeIndex = 0;
  const u32 normalAttributeIndex = 1;
  const u32 texture0AttributeIndex = 2;

  glGenVertexArrays(1, &model->vertexAtt.arrayObject);
  glGenBuffers(1, &model->vertexAtt.bufferObject);
  glGenBuffers(1, &model->vertexAtt.indexObject);

  glBindVertexArray(model->vertexAtt.arrayObject);
  glBindBuffer(GL_ARRAY_BUFFER, model->vertexAtt.bufferObject);
  glBufferData(GL_ARRAY_BUFFER,
               sizeOfAttributeData,
               gltfModel->buffers[vertexAttBufferIndex].data.data(),
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
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->vertexAtt.indexObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesGLTFBufferByteLength, dataOffset, GL_STATIC_DRAW);

  // unbind VBO & VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void loadModelTexture(u32* textureId, tinygltf::Image* image, b32 inputSRGB = false)
{
  glGenTextures(1, textureId);
  glBindTexture(GL_TEXTURE_2D, *textureId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // disables bilinear filtering (creates sharp edges when magnifying texture)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  u8* imageData = image->image.data();
  u32 numComponents = image->component;

  // load image data
  if (!image->image.empty() && numComponents <= 4)
  {
    u32 dataColorSpace;
    u32 dataComponentComposition;
    switch(numComponents) {
      case 1:
        dataColorSpace = dataComponentComposition = GL_RED;
        break;
      case 2:
        dataColorSpace = dataComponentComposition = GL_RG;
        break;
      case 3:
        dataColorSpace = inputSRGB ? GL_SRGB : GL_RGB;
        dataComponentComposition = GL_RGB;
        break;
      case 4:
        dataColorSpace = inputSRGB ? GL_SRGB_ALPHA : GL_RGBA;
        dataComponentComposition = GL_RGBA;
        break;
      default:
        Assert(false);
    }

    glTexImage2D(GL_TEXTURE_2D, // target
                 0, // level of detail (level n is the nth mipmap reduction image)
                 dataColorSpace, // What is the color space of the data
                 image->width, // width of texture
                 image->height, // height of texture
                 0, // border (legacy stuff, MUST BE 0)
                 dataComponentComposition, // How are the components of the data composed
                 GL_UNSIGNED_BYTE, // specifies data type of pixel data
                 imageData); // pointer to the image data
    glGenerateMipmap(GL_TEXTURE_2D);

    // set texture options
  } else
  {
    std::cout << "Failed to load texture" << std::endl;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
}

TextureData initializeModelTextureData(tinygltf::Model* model) {
  TextureData resultTextureData;

  // TODO: handle more then just drawing the first mesh
  Assert(!model->meshes.empty());
  Assert(!model->meshes[0].primitives.empty());
  u32 materialIndex = model->meshes[0].primitives[0].material;
  tinygltf::Material material = model->materials[materialIndex];
  // TODO: Handle more then just TEXCOORD_0 vertex attribute?
  Assert(material.normalTexture.texCoord == 0 && material.pbrMetallicRoughness.baseColorTexture.texCoord == 0)
  u32 normalTextureIndex = material.normalTexture.index;
  u32 baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;

  // NOTE: gltf.textures.samplers gives info about how to magnify/minify textures and how texture wrapping should work
  u32 normalImageIndex = model->textures[normalTextureIndex].source;
  tinygltf::Image normalImage = model->images[normalImageIndex];
  u32 albedoColorImageIndex = model->textures[baseColorTextureIndex].source;
  tinygltf::Image albedoImage = model->images[albedoColorImageIndex];

  loadModelTexture(&resultTextureData.normalTextureId, &normalImage);
  loadModelTexture(&resultTextureData.albedoTextureId, &albedoImage);

  return resultTextureData;
}

Model loadModel(const char* filePath) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  tinygltf::Model tinyGLTFModel;
  Model resultModel;

  //bool ret = loader.LoadASCIIFromFile(&tinyGLTFModel, &err, &warn, filePath); // for .gltf
  bool ret = loader.LoadBinaryFromFile(&tinyGLTFModel, &err, &warn, filePath); // for binary glTF(.glb)

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
  }

  initializeModelVertexData(&tinyGLTFModel, &resultModel);
  resultModel.textureData = initializeModelTextureData(&tinyGLTFModel);

  return resultModel;
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

  Model vertexAttModel;
  for(u32 i = 0; i < modelCount; ++i) {
    initializeModelVertexData(&models[i], &vertexAttModel);
    *(returnVertAtts[i]) = vertexAttModel.vertexAtt;
  }
}