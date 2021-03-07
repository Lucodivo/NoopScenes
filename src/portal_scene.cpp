#include <glm/glm.hpp>

#include <time.h>

#include "vertex_attributes.h"
#include "shader_program.h"
#include "file_locations.h"
#include "glfw_util.cpp"

const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

struct Camera {
  glm::vec3 origin;
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 forward;
  f32 pitch; //
  f32 yaw;
};

struct StopWatch {
  f32 lastFrame;
  f32 delta;
};

f32 getTime() {
  clock_t time = clock();
  return (f32)time / CLOCKS_PER_SEC;
}

// NOTE: pitch and yaw are set to radians
// NOTE: There is currently no support for lookAt where forward should point directly up
Camera lookAt(glm::vec3 origin, glm::vec3 focus) {
  const f32 forwardDotUpThresholdMax = 0.996194f; // cos(5 degrees)
  const f32 forwardDotUpThresholdMin = -0.996194f; // cos(175 degrees)

  Camera camera;
  camera.origin = origin;
  camera.forward = glm::normalize(focus - origin);
  f32 forwardDotUp = glm::dot(camera.forward, worldUp);
  if (forwardDotUp > forwardDotUpThresholdMax || forwardDotUp < forwardDotUpThresholdMin)
  {
    std::cout << "Look At Camera Failed" << std::endl;
    camera.forward = glm::vec3(0.0f, 0.0f, -1.0f);
  }

  camera.pitch = glm::asin(camera.forward.y);

  glm::vec2 cameraForwardXZPlane = glm::normalize(glm::vec2(camera.forward.x, camera.forward.z));
  f32 cameraFrontDotZeroPitch = glm::dot(cameraForwardXZPlane, glm::vec2(0.0f, -1.0f));
  camera.yaw = glm::acos(cameraFrontDotZeroPitch);

  camera.right = glm::normalize(glm::cross(worldUp, camera.forward));
  camera.up = glm::cross(camera.forward, camera.right);
  return camera;
}

void rotateCamera(Camera* camera, f32 pitchOffset, f32 yawOffset) {
  const f32 maxMinPitch = RadiansPerDegree * 85.0f;

  camera->pitch += pitchOffset;
  if(camera->pitch > maxMinPitch) {
    camera->pitch = maxMinPitch;
  } else if(camera->pitch < -maxMinPitch){
    camera->pitch = -maxMinPitch;
  }

  camera->yaw += yawOffset;
  if(camera->yaw > Tau32) {
    camera->yaw -= Tau32;
  }

  // Calculate the new Front vector
  glm::vec3 forward;
  f32 cosPitch = cos(camera->pitch);
  forward.x = sin(camera->yaw) * cosPitch;
  forward.y = sin(camera->pitch);
  forward.z = -cos(camera->yaw) * cosPitch;

  camera->forward = glm::normalize(forward);
  // Also re-calculate the Right and Up vector
  camera->right = glm::normalize(glm::cross(camera->forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
  camera->up = glm::normalize(glm::cross(camera->right, camera->forward));
}

// NOTE: offsetPitch and offsetYaw in radians
glm::mat4 getViewMatrix(Camera camera) {
  // In glm we access elements as mat[col][row] due to column-major layout
  glm::mat4 translation = glm::mat4(
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          -camera.origin.x, -camera.origin.y, -camera.origin.z, 1.0f);

  glm::mat4 rotation = glm::mat4(
          camera.right.x, camera.up.x, -camera.forward.x, 0.0f,
          camera.right.y, camera.up.y, -camera.forward.y, 0.0f,
          camera.right.z, camera.up.z, -camera.forward.z, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f);

  // Return lookAt matrix as combination of translation and rotation matrix
  return rotation * translation; // Remember to read from right to left (first translation then rotation)
}

StopWatch createStopWatch() {
  StopWatch stopWatch;
  stopWatch.lastFrame = getTime();
  stopWatch.delta = 0.0f;
  return stopWatch;
}

void updateStopWatch(StopWatch* stopWatch) {
  f32 t = getTime();
  stopWatch->delta = t - stopWatch->lastFrame;
  stopWatch->lastFrame = t;
}

Extent2D toggleWindowSize(GLFWwindow* window, const u32 width, const u32 height)
{
  Extent2D resultWindowExtent{ width, height };
  local_access bool windowMode = true;
  if (windowMode) {
    resultWindowExtent = toFullScreenMode(window);
  } else{
    toWindowedMode(window, width, height);
  }
  windowMode = !windowMode;
  return resultWindowExtent;
}

void portalScene(GLFWwindow* window) {
  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;
  Camera camera = lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  ShaderProgram cubeShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);

  glm::mat4 cubeModelMatrix = glm::mat4(1.0f);
  glm::mat4 tmpCubeModelMatrix;
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

  glm::vec3 cubeColor = glm::vec3(0.9f, 0.9f, 0.9f);
  glm::vec3 wireFrameColor = glm::vec3(0.1f, 0.1f, 0.1f);

  glClearColor(0.3f, 0.1f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  glLineWidth(3.0f);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  glBindVertexArray(cubePosVertexAtt.arrayObject);
  glViewport(0, 0, windowExtent.width, windowExtent.height);

  glUseProgram(cubeShader.id);
  setUniform(cubeShader.id, "projection", projectionMatrix);

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

    if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
      windowExtent = toggleWindowSize(window, initWindowExtent.width, initWindowExtent.height);
      aspectRatio = f32(windowExtent.width) / windowExtent.height;
      glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
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

    cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(30.0f * stopWatch.delta), glm::vec3(0.0f, 1.0f, 0.0f));

    // draw
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rotateCamera(&camera, -mouseDelta.y * 0.001, mouseDelta.x * 0.001);
    viewMatrix = getViewMatrix(camera);
    setUniform(cubeShader.id, "view", viewMatrix);
    setUniform(cubeShader.id, "model", cubeModelMatrix);

    { // draw cube
      setUniform(cubeShader.id, "color", cubeColor);
      glDrawElements(GL_TRIANGLES, // drawing mode
                   36, // number of elements to draw (3 vertices per triangle * 2 triangles per face * 6 faces)
                   GL_UNSIGNED_INT, // type of the indices
                   0); // offset in the EBO
    }

    { // draw wireframe
      glDisable(GL_DEPTH_TEST);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);
      setUniform(cubeShader.id, "color", wireFrameColor);
      glDrawElements(GL_TRIANGLES, // drawing mode
                     36, // number of elements to draw (3 vertices per triangle * 2 triangles per face * 6 faces)
                     GL_UNSIGNED_INT, // type of the indices
                     0); // offset in the EBO
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);
    }

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(cubeShader);
  deleteVertexAtt(cubePosVertexAtt);
}