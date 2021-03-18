#include <glm/glm.hpp>

#include "vertex_attributes.h"
#include "shader_program.h"
#include "file_locations.h"
#include "glfw_util.cpp"
#include "util.h"
#include "textures.h"
#include "camera.h"

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

  auto populateAttributeMetadata = [primitive, model](const char* keyString) -> gltfAttributeMetadata {
    gltfAttributeMetadata result;
    result.accessorIndex = primitive.attributes.at(keyString);
    result.numComponents = tinygltf::GetNumComponentsInType(model->accessors[result.accessorIndex].type);
    result.bufferViewIndex = model->accessors[result.accessorIndex].bufferView;
    result.bufferIndex = model->bufferViews[result.bufferViewIndex].buffer;
    result.bufferByteOffset = model->bufferViews[result.bufferViewIndex].byteOffset;
    result.bufferByteLength = model->bufferViews[result.bufferViewIndex].byteLength;
    return result;
  };

  // TODO: Allow variability in attributes beyond POSITION, NORMAL, TEXCOORD_0
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
  u32 indicesGLTFBufferViewIndex = model->accessors[indicesAccessorIndex].bufferView;
  u32 indicesGLTFBufferIndex = model->bufferViews[indicesGLTFBufferViewIndex].buffer;
  u64 indicesGLTFBufferByteOffset = model->bufferViews[indicesGLTFBufferViewIndex].byteOffset;
  u64 indicesGLTFBufferByteLength = model->bufferViews[indicesGLTFBufferViewIndex].byteLength;

  u8* dataOffset = model->buffers[indicesGLTFBufferIndex].data.data() + indicesGLTFBufferByteOffset;

  VertexAtt vertexAtt;
  vertexAtt.indexCount = u32(model->accessors[indicesAccessorIndex].count);
  vertexAtt.indexTypeSizeInBytes = tinygltf::GetComponentSizeInBytes(model->accessors[indicesAccessorIndex].componentType);
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

void loadModelsVertexAtt(const char** filePaths, VertexAtt** returnVertAtts, u32 count) {
  tinygltf::TinyGLTF loader;

  for(u32 i = 0; i < count; ++i) {
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

    *(returnVertAtts[i]) = initializeModelVertexBuffer(&model);
  }
}

void portalScene(GLFWwindow* window) {
  ShaderProgram modelShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);

  VertexAtt gateModelVertAtt, pyramidVertAtt, crystalModelVertAtt, icosphereModelVertAtt, torusTriangleModelVertAtt;
  VertexAtt* modelPtrs[] = {&gateModelVertAtt, &pyramidVertAtt, &crystalModelVertAtt, &icosphereModelVertAtt, &torusTriangleModelVertAtt };
  const char* modelLocs[] = { gateModelLoc, pyramidModelLoc, crystalModelLoc, icosphere1ModelLoc, torusTriangleModelLoc };
  loadModelsVertexAtt(modelLocs, modelPtrs, ArrayCount(modelPtrs));

  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  Camera camera = lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers(false);
  VertexAtt invertedCubePosVertexAtt = initializeCubePositionVertexAttBuffers(true);

  const f32 cubeScale = 1.0f;
  const glm::vec3 cubePosition = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 wireFrameColor = glm::vec3(0.1f, 0.1f, 0.1f);

  const f32 portalScale = 2.0f;
  const glm::vec3 portalPosition = glm::vec3(0.0f, 0.0f, 0.0f);

  const f32 gateScale = portalScale;

  glm::mat4 shapeModelMatrix = glm::translate(glm::mat4(glm::mat3(cubeScale)), cubePosition);
  glm::mat4 portalModelMatrix = glm::translate(glm::mat4(glm::mat3(portalScale)), portalPosition);
  glm::mat4 gateModelMatrix = glm::mat4(glm::mat3(gateScale));
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, 0.1f, 100.0f);

  GLuint skyboxTexture1Id, skyboxTexture2Id, skyboxTexture3Id, skyboxTexture4Id;
  loadCubeMapTexture(skyboxWaterFaceLocations, &skyboxTexture1Id);
  loadCubeMapTexture(skyboxInterstellarFaceLocations, &skyboxTexture2Id);
  loadCubeMapTexture(skyboxSpaceLightBlueFaceLocations , &skyboxTexture3Id);
  loadCubeMapTexture(skyboxYellowCloudFaceLocations, &skyboxTexture4Id);

  s32 skyboxTexture1Index = 0;
  s32 skyboxTexture2Index = 1;
  s32 skyboxTexture3Index = 2;
  s32 skyboxTexture4Index = 3;
  glActiveTexture(GL_TEXTURE0 + skyboxTexture1Index);
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture1Id);
  glActiveTexture(GL_TEXTURE0 + skyboxTexture2Index);
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture2Id);
  glActiveTexture(GL_TEXTURE0 + skyboxTexture3Index);
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture3Id);
  glActiveTexture(GL_TEXTURE0 + skyboxTexture4Index);
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture4Id);

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_STENCIL_TEST);

  glLineWidth(3.0f);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glEnable(GL_STENCIL_TEST);

  glViewport(0, 0, windowExtent.width, windowExtent.height);

  glUseProgram(modelShader.id);
  setUniform(modelShader.id, "projection", projectionMatrix);

  glUseProgram(skyboxShader.id);
  setUniform(skyboxShader.id, "projection", projectionMatrix);

  StopWatch stopWatch = createStopWatch();
  while(glfwWindowShouldClose(window) == GL_FALSE)
  {
    loadInputStateForFrame(window);
    updateStopWatch(&stopWatch);

    if (isActive(KeyboardInput_Esc))
    {
      glfwSetWindowShouldClose(window, true);
      break;
    }

    // toggle fullscreen/window mode if alt + enter
    if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
      windowExtent = toggleWindowSize(window, initWindowExtent.width, initWindowExtent.height);
      aspectRatio = f32(windowExtent.width) / windowExtent.height;
      glm::mat4 projectionMatrix = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, 0.1f, 100.0f);
      glViewport(0, 0, windowExtent.width, windowExtent.height);
      setUniform(modelShader.id, "projection", projectionMatrix);
    }

    // gather input
    b32 leftShiftIsActive = isActive(KeyboardInput_Shift_Left);
    b32 leftIsActive = isActive(KeyboardInput_A) || isActive(KeyboardInput_Left);
    b32 rightIsActive = isActive(KeyboardInput_D) || isActive(KeyboardInput_Right);
    b32 upIsActive = isActive(KeyboardInput_W) || isActive(KeyboardInput_Up);
    b32 downIsActive = isActive(KeyboardInput_S) || isActive(KeyboardInput_Down);
    Vec2_f64 mouseDelta = getMouseDelta();

    // use input to modify camera
    {
      b32 lateralMovement = leftIsActive != rightIsActive;
      b32 forwardMovement = upIsActive != downIsActive;
      if (lateralMovement || forwardMovement)
      {
        f32 cameraMovementSpeed = leftShiftIsActive ? 2.0f : 1.0f;

        // Camera movement direction
        glm::vec3 cameraMovementDirection{};
        if (lateralMovement)
        {
          cameraMovementDirection += rightIsActive ? camera.right : -camera.right;
        }

        if (forwardMovement)
        {
          cameraMovementDirection += upIsActive ? camera.forward : -camera.forward;
        }

        cameraMovementDirection = glm::normalize(glm::vec3(cameraMovementDirection.x, 0.0f, cameraMovementDirection.z));
        camera.origin += cameraMovementDirection * cameraMovementSpeed * stopWatch.delta;
      }

      rotateCamera(&camera, f32(-mouseDelta.y * 0.001), f32(mouseDelta.x * 0.001));
      viewMatrix = getViewMatrix(camera);
    }

    // draw
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // draw gate
    {
      glStencilFunc(GL_ALWAYS, // stencil function always passes
                    0x00, // reference
                    0x00); // mask
      glUseProgram(modelShader.id);
      setUniform(modelShader.id, "view", viewMatrix);
      setUniform(modelShader.id, "model", gateModelMatrix);

      // draw cube
      setUniform(modelShader.id, "color", glm::vec3(0.4f, 0.4f, 0.4f));
      drawTriangles(gateModelVertAtt);
    }

    { // draw out stencils
      // GL_EQUAL
      // Passes if ( ref & mask ) == ( stencil & mask )
      // Only draw portals where the stencil is cleared
      glStencilFunc(GL_EQUAL, // func
                    0xFF, // ref
                    0x00); // mask
      glStencilOp(GL_KEEP, // action when stencil fails
                  GL_KEEP, // action when stencil passes but depth fails
                  GL_REPLACE); // action when both stencil and depth pass

      glUseProgram(modelShader.id);
      setUniform(modelShader.id, "view", viewMatrix);
      setUniform(modelShader.id, "model", portalModelMatrix);

      glStencilMask(0x01);
      drawTriangles(cubePosVertexAtt, 6, 0);

      glStencilMask(0x02);
      drawTriangles(cubePosVertexAtt, 6, 6);

      glStencilMask(0x03);
      drawTriangles(cubePosVertexAtt, 6, 24);

      glStencilMask(0x04);
      drawTriangles(cubePosVertexAtt, 6, 30);
    }

    // turn off writes to the stencil
    glStencilMask(0x00);

    // We need to clear disable depth values so distant objects through the "portals" still get drawn
    // The portals themselves will still obey the depth of the scene, as the stencils have been rendered with depth in mind
    glClear(GL_DEPTH_BUFFER_BIT);
    { // use stencils to draw portals
      glm::mat4 skyboxViewMat = glm::mat4(glm::mat3(viewMatrix));

      shapeModelMatrix = glm::rotate(shapeModelMatrix,
                                     30.0f * RadiansPerDegree * stopWatch.delta,
                                     glm::vec3(0.0f, 1.0f, 0.0f));


      glUseProgram(modelShader.id);
      setUniform(modelShader.id, "view", viewMatrix);
      setUniform(modelShader.id, "model", shapeModelMatrix);

      glUseProgram(skyboxShader.id);
      setUniform(skyboxShader.id, "view", skyboxViewMat);

      // portal 1
      {
        glUseProgram(skyboxShader.id);
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x01, // ref
                      0xFF); // enable which bits in reference and stored value are compared
        setUniform(skyboxShader.id, "skybox", skyboxTexture1Index);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(modelShader.id);
        setUniform(modelShader.id, "color", glm::vec3(0.4, 0.4, 1.0));
        drawTriangles(torusTriangleModelVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(modelShader.id, "color", wireFrameColor);
        drawTriangles(torusTriangleModelVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }

      // portal 2
      {
        glUseProgram(skyboxShader.id);
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x02, // ref
                      0xFF); // enable which bits in reference and stored value are compared
        setUniform(skyboxShader.id, "skybox", skyboxTexture2Index);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(modelShader.id);
        setUniform(modelShader.id, "color", glm::vec3(0.4, 1.0, 0.4));
        drawTriangles(crystalModelVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(modelShader.id, "color", wireFrameColor);
        drawTriangles(crystalModelVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }

      // portal 3
      {
        glUseProgram(skyboxShader.id);
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x03, // ref
                      0xFF); // enable which bits in reference and stored value are compared
        setUniform(skyboxShader.id, "skybox", skyboxTexture3Index);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(modelShader.id);
        setUniform(modelShader.id, "color", glm::vec3(0.9, 0.9, 0.9));
        drawTriangles(icosphereModelVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(modelShader.id, "color", wireFrameColor);
        drawTriangles(icosphereModelVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }

      // portal 4
      {
        glUseProgram(skyboxShader.id);
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x04, // ref
                      0xFF); // enable which bits in reference and stored value are compared
        setUniform(skyboxShader.id, "skybox", skyboxTexture4Index);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(modelShader.id);
        setUniform(modelShader.id, "color", glm::vec3(1.0, 0.4, 0.4));
        drawTriangles(pyramidVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(modelShader.id, "color", wireFrameColor);
        drawTriangles(pyramidVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }
    }

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(modelShader);
  deleteShaderProgram(skyboxShader);
  deleteVertexAtts(ArrayCount(modelPtrs), modelPtrs);
}