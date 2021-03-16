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
  const char* positionIndexKeyString = "POSITION";
  const char* normalIndexKeyString = "NORMAL";
  const char* texture0IndexKeyString = "TEXCOORD_0";

  // TODO: Pull vertex attributes for more then the first primitive of the first mesh of a model
  // TODO: Allow variability in attributes and not just POSITION, NORMAL, TEXCOORD_0
  Assert(!model->meshes.empty())
  Assert(!model->meshes[0].primitives.empty())
  Assert(!model->meshes[0].primitives[0].attributes.empty())
  u32 positionAccessorIndex = model->meshes[0].primitives[0].attributes[positionIndexKeyString];
  u32 normalAccessorIndex = model->meshes[0].primitives[0].attributes[normalIndexKeyString];
  u32 texture0AccessorIndex = model->meshes[0].primitives[0].attributes[texture0IndexKeyString];

  u32 positionNumComponents = tinygltf::GetNumComponentsInType(model->accessors[positionAccessorIndex].type);
  u32 normalNumComponents = tinygltf::GetNumComponentsInType(model->accessors[normalAccessorIndex].type);
  u32 texture0NumComponents = tinygltf::GetNumComponentsInType(model->accessors[texture0AccessorIndex].type);

  u32 positionGLTFBufferViewIndex = model->accessors[positionAccessorIndex].bufferView;
  u32 normalGLTFBufferViewIndex = model->accessors[normalAccessorIndex].bufferView;
  u32 texture0GLTFBufferViewIndex = model->accessors[texture0AccessorIndex].bufferView;

  u32 positionGLTFBufferIndex = model->bufferViews[positionGLTFBufferViewIndex].buffer;
  u32 normalGLTFBufferIndex = model->bufferViews[normalGLTFBufferViewIndex].buffer;
  u32 texture0GLTFBufferIndex = model->bufferViews[texture0GLTFBufferViewIndex].buffer;

  // TODO: Handle vertex attributes that don't share the same buffer
  Assert(positionGLTFBufferIndex == normalGLTFBufferIndex && normalGLTFBufferIndex == texture0GLTFBufferIndex);

  VertexAtt vertexAtt;
  vertexAtt.vertexCount = model->accessors[positionAccessorIndex].count;
  u32 sizeOfAttributeData = sizeof(f32) * (vertexAtt.vertexCount * (positionNumComponents + normalNumComponents + texture0NumComponents));
  const u32 positionAttributeIndex = 0;
  const u32 normalAttributeIndex = 1;
  const u32 textureAttributeIndex = 2;

//  glGenVertexArrays(1, &vertexAtt.arrayObject);
//  glGenBuffers(1, &vertexAtt.bufferObject);
//
//  glBindVertexArray(vertexAtt.arrayObject);
//
//  glBindBuffer(GL_ARRAY_BUFFER, vertexAtt.bufferObject);
//  glBufferData(GL_ARRAY_BUFFER,
//               sizeof(quadPosTexVertexAttributes),
//               quadPosTexVertexAttributes,
//               GL_STATIC_DRAW);
//
//  // set the vertex attributes (position and texture)
//  // position attribute
//  glVertexAttribPointer(positionAttributeIndex,
//                        2, // attribute size
//                        GL_FLOAT,
//                        GL_FALSE,
//                        quadPosTexVertexAttSizeInBytes,
//                        (void*)0);
//  glEnableVertexAttribArray(positionAttributeIndex);
//
//  // texture attribute
//  glVertexAttribPointer(textureAttributeIndex,
//                        2, // attribute size
//                        GL_FLOAT,
//                        GL_FALSE,
//                        quadPosTexVertexAttSizeInBytes,
//                        (void*)(3 * sizeof(f32)));
//  glEnableVertexAttribArray(textureAttributeIndex);
//
//  // unbind VBO, VAO, & EBO
//  glBindBuffer(GL_ARRAY_BUFFER, 0);
//  glBindVertexArray(0);

  return vertexAtt;
}

VertexAtt loadModelVertexAtt(const char* filePath) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  //bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filePath); // for .gltf
  bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath); // for binary glTF(.glb)

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
  }

  return initializeModelVertexBuffer(&model);
}

void portalScene(GLFWwindow* window) {
  ShaderProgram cubeShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);

  VertexAtt modelVertexAtt = loadModelVertexAtt(portalModelLoc);

  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  Camera camera = lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  VertexAtt quadVertexAtt = initializeQuadPosTexVertexAttBuffers();

  const f32 cubeScale = 1.0f;
  const glm::vec3 cubePosition = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cubeColor = glm::vec3(0.9f, 0.9f, 0.9f);
  glm::vec3 wireFrameColor = glm::vec3(0.1f, 0.1f, 0.1f);

  const f32 portalScale = 2.0f;
  const glm::vec3 portalPosition = glm::vec3(0.0f, 0.0f, 0.0f);

  glm::mat4 cubeModelMatrix = glm::translate(glm::mat4(glm::mat3(cubeScale)), cubePosition);
  glm::mat4 portalModelMatrix = glm::translate(glm::mat4(glm::mat3(portalScale)), portalPosition);
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, 0.1f, 100.0f);

  GLuint skyboxTexture1Id, skyboxTexture2Id, skyboxTexture3Id, skyboxTexture4Id;
  loadCubeMapTexture(skyboxYellowCloudFaceLocations, &skyboxTexture1Id);
  loadCubeMapTexture(skyboxInterstellarFaceLocations, &skyboxTexture2Id);
  loadCubeMapTexture(skyboxWaterFaceLocations, &skyboxTexture3Id);
  loadCubeMapTexture(skyboxSpaceLightBlueFaceLocations, &skyboxTexture4Id);

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

  glUseProgram(cubeShader.id);
  setUniform(cubeShader.id, "projection", projectionMatrix);

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
      setUniform(cubeShader.id, "projection", projectionMatrix);
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

    { // draw portal
      glStencilFunc(GL_NEVER, // stencil function never passes, so never draws
                    0xFF,
                    0xFF);
      // GL_LESS
      // Passes if ( ref & mask ) < ( stencil & mask )
      glStencilOp(GL_REPLACE, // action when stencil fails
                  GL_KEEP, // action when stencil passes but depth fails
                  GL_REPLACE); // action when both stencil and depth pass

      glUseProgram(cubeShader.id);
      setUniform(cubeShader.id, "view", viewMatrix);
      setUniform(cubeShader.id, "model", portalModelMatrix);

      glStencilMask(0x01);
      drawIndexedTriangles(cubePosVertexAtt, 6, 0);

      glStencilMask(0x02);
      drawIndexedTriangles(cubePosVertexAtt, 6, 6);

      glStencilMask(0x03);
      drawIndexedTriangles(cubePosVertexAtt, 6, 24);

      glStencilMask(0x04);
      drawIndexedTriangles(cubePosVertexAtt, 6, 30);
    }

    // turn off writes to the stencil
    glStencilMask(0x00);

    { // skyboxes
      glm::mat4 skyboxViewMat = glm::mat4(glm::mat3(viewMatrix));

      glFrontFace(GL_CW);
      glUseProgram(skyboxShader.id);
      setUniform(skyboxShader.id, "view", skyboxViewMat);

      // portal skybox 1
      glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                    0x01, // ref
                    0xFF); // enable which bits in reference and stored value are compared
      setUniform(skyboxShader.id, "skybox", skyboxTexture1Index);
      drawIndexedTriangles(cubePosVertexAtt);

      // portal skybox 2
      glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                    0x02, // ref
                    0xFF); // enable which bits in reference and stored value are compared
      setUniform(skyboxShader.id, "skybox", skyboxTexture2Index);
      drawIndexedTriangles(cubePosVertexAtt);

      // portal skybox 3
      glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                    0x03, // ref
                    0xFF); // enable which bits in reference and stored value are compared
      setUniform(skyboxShader.id, "skybox", skyboxTexture3Index);
      drawIndexedTriangles(cubePosVertexAtt);

      // portal skybox 4
      glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                    0x04, // ref
                    0xFF); // enable which bits in reference and stored value are compared
      setUniform(skyboxShader.id, "skybox", skyboxTexture4Index);
      drawIndexedTriangles(cubePosVertexAtt);

      glFrontFace(GL_CCW);
    }

    { // cube
      cubeModelMatrix = glm::rotate(cubeModelMatrix,
                                    30.0f * RadiansPerDegree * stopWatch.delta,
                                    glm::vec3(0.0f, 1.0f, 0.0f));

      glStencilFunc(GL_LEQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                    0x01, // ref
                    0xFF); // enable which bits in reference and stored value are compared

      glUseProgram(cubeShader.id);
      setUniform(cubeShader.id, "view", viewMatrix);
      setUniform(cubeShader.id, "model", cubeModelMatrix);

      // draw cube
      setUniform(cubeShader.id, "color", cubeColor);
      drawIndexedTriangles(cubePosVertexAtt);

      // draw wireframe
      glDisable(GL_DEPTH_TEST);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);
      setUniform(cubeShader.id, "color", wireFrameColor);
      drawIndexedTriangles(cubePosVertexAtt);
      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_DEPTH_TEST);
    }

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(cubeShader);
  VertexAtt vertexAtts[] = {cubePosVertexAtt, quadVertexAtt};
  deleteVertexAtts(ArrayCount(vertexAtts), vertexAtts);
}