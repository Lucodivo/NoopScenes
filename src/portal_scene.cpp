#include <glm/glm.hpp>

#include "vertex_attributes.h"
#include "shader_program.h"
#include "file_locations.h"
#include "glfw_util.cpp"
#include "util.h"
#include "textures.h"
#include "camera.h"

void portalScene(GLFWwindow* window) {
  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  Camera camera = lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f));

  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  ShaderProgram cubeShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);
  VertexAtt quadVertexAtt = initializeQuadPosTexVertexAttBuffers();

  const f32 cubeScale = 2.0f;
  glm::vec3 cubeColor = glm::vec3(0.9f, 0.9f, 0.9f);
  glm::vec3 wireFrameColor = glm::vec3(0.1f, 0.1f, 0.1f);

  const f32 quadScale = 2.0f;
  const glm::vec3 quadAspectRatioScale = glm::vec3(quadScale * aspectRatio, quadScale * 1.0f, 1.0f);
  const glm::vec3 quadPosition = glm::vec3(0.0f, 0.0f, 5.0f);

  glm::mat4 cubeModelMatrix = glm::mat4(cubeScale);
  glm::mat4 quadModelMatrix = glm::translate(glm::scale(glm::mat4(1.0), quadAspectRatioScale), quadPosition);
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, 0.1f, 100.0f);

  GLuint skyboxTextureId;
  loadCubeMapTexture(yellowCloudFaceLocations, &skyboxTextureId);

  u32 skyboxTextureIndex = 0;
  glActiveTexture(GL_TEXTURE0 + skyboxTextureIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureId);

  glClearColor(0.3f, 0.1f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

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
  setUniform(skyboxShader.id, "skybox", skyboxTextureIndex);

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

        cameraMovementDirection = glm::normalize(cameraMovementDirection);
        camera.origin += cameraMovementDirection * cameraMovementSpeed * stopWatch.delta;
      }

      rotateCamera(&camera, -mouseDelta.y * 0.001, mouseDelta.x * 0.001);
      viewMatrix = getViewMatrix(camera);
    }

    // draw
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    { // draw portal
      glUseProgram(cubeShader.id);
      setUniform(cubeShader.id, "view", viewMatrix);
      setUniform(cubeShader.id, "model", quadModelMatrix);
      setUniform(cubeShader.id, "color", glm::vec3(0.2, 0.5, 0.3));
      drawIndexedTriangles(quadVertexAtt);
    }

    { // skybox
      glm::mat4 skyboxViewMat = glm::mat4(glm::mat3(viewMatrix));

      glUseProgram(skyboxShader.id);
      glBindVertexArray(cubePosVertexAtt.arrayObject);
      setUniform(skyboxShader.id, "view", skyboxViewMat);
      glFrontFace(GL_CW);

      setUniform(skyboxShader.id, "color", glm::vec3(0.2, 0.5, 0.3));
      drawIndexedTriangles(cubePosVertexAtt);

      glFrontFace(GL_CCW);
    }

    { // cube
      cubeModelMatrix = glm::rotate(cubeModelMatrix,
                                    30.0f * RadiansPerDegree * stopWatch.delta,
                                    glm::vec3(0.0f, 1.0f, 0.0f));

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