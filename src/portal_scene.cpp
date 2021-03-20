#include <glm/glm.hpp>

#include "vertex_attributes.h"
#include "shader_program.h"
#include "file_locations.h"
#include "glfw_util.cpp"
#include "util.h"
#include "textures.h"
#include "camera.h"
#include "uniform_buffer_object_structs.h"
#include "model.h"

const glm::vec3 defaultPlayerDimensionInMeters = glm::vec3(0.5f, 1.75f, 0.25f); // ~1'7"w, 6'h, 9"d

struct Player {
  glm::vec3 minBoxPosition;
  glm::vec3 dimensionInMeters;
  // assume "eyes" (FPS camera) is positioned on front Z, top Y, middle X
};

void portalScene(GLFWwindow* window) {
  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  ShaderProgram gateShader = createShaderProgram(posNormTexVertexShaderFileLoc, portalFragmentShaderFileLoc);
  ShaderProgram shapeShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);

  ProjectionViewModelUBO projectionViewModelUbo;
  Player player;
  player.minBoxPosition = glm::vec3(0.0f, 0.0f, 10.0f);
  player.dimensionInMeters = defaultPlayerDimensionInMeters;

  VertexAtt gateModelVertAtt, pyramidVertAtt, crystalModelVertAtt, icosphereModelVertAtt, torusPentagonModelVertAtt;
  VertexAtt* modelPtrs[] = { &gateModelVertAtt, &pyramidVertAtt, &crystalModelVertAtt, &icosphereModelVertAtt, &torusPentagonModelVertAtt };
  const char* modelLocs[] = { gateModelLoc, pyramidModelLoc, crystalModelLoc, icosphere1ModelLoc, torusPentagonModelLoc };
  loadModelsVertexAtt(modelLocs, modelPtrs, ArrayCount(modelPtrs));

  const f32 portalScale = 3.0f;
  const glm::vec3 portalPosition = glm::vec3(0.0f, portalScale * 0.5f, 0.0);

  const f32 gateScale = portalScale;
  const glm::vec3 gatePosition = portalPosition;

  const f32 shapeScale = 1.5f;
  const glm::vec3 shapePosition = portalPosition;
  glm::vec3 wireFrameColor = glm::vec3(0.1f, 0.1f, 0.1f);

  glm::vec3 cameraPosition = player.minBoxPosition + (player.dimensionInMeters * glm::vec3(0.5f, 1.0f, 1.0f));
  Camera firstPersonCamera = lookAt(cameraPosition, gatePosition);
  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  VertexAtt invertedCubePosVertexAtt = initializeCubePositionVertexAttBuffers(true);

  glm::mat4 shapeModelMatrix = glm::translate(glm::mat4(glm::mat3(1.0f)), shapePosition) * glm::mat4(glm::mat3(shapeScale));
  glm::mat4 portalModelMatrix = glm::translate(glm::mat4(glm::mat3(1.0f)), portalPosition) * glm::mat4(glm::mat3(portalScale));
  glm::mat4 gateModelMatrix = glm::translate(glm::mat4(glm::mat3(1.0f)), gatePosition) * glm::mat4(glm::mat3(gateScale));
  projectionViewModelUbo.projection = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, 0.1f, 100.0f);

  GLuint mainSkyboxTextureId, portal1SkyboxTextureId, portal2SkyboxTextureId, portal3SkyboxTextureId, portal4SkyboxTextureId;
  loadCubeMapTexture(caveFaceLocations, &mainSkyboxTextureId);
  loadCubeMapTexture(calmSeaFaceLocations, &portal1SkyboxTextureId);
  loadCubeMapTexture(skyboxInterstellarFaceLocations, &portal2SkyboxTextureId);
  loadCubeMapTexture(pollutedEarthFaceLocations , &portal3SkyboxTextureId);
  loadCubeMapTexture(skyboxYellowCloudFaceLocations, &portal4SkyboxTextureId);

  s32 mainSkyboxTextureIndex = 0;
  s32 portal1SkyboxTextureIndex = 1;
  s32 portal2SkyboxTextureIndex = 2;
  s32 portal3SkyboxTextureIndex = 3;
  s32 portal4SkyboxTextureIndex = 4;
  glActiveTexture(GL_TEXTURE0 + mainSkyboxTextureIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, mainSkyboxTextureId);
  glActiveTexture(GL_TEXTURE0 + portal1SkyboxTextureIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, portal1SkyboxTextureId);
  glActiveTexture(GL_TEXTURE0 + portal2SkyboxTextureIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, portal2SkyboxTextureId);
  glActiveTexture(GL_TEXTURE0 + portal3SkyboxTextureIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, portal3SkyboxTextureId);
  glActiveTexture(GL_TEXTURE0 + portal4SkyboxTextureIndex);
  glBindTexture(GL_TEXTURE_CUBE_MAP, portal4SkyboxTextureId);

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

  u32 portalFragUBOBindingIndex = 1;
  struct PortalFragUBO {
    glm::vec3 directionalLightColor;
    u8 __padding1;
    glm::vec3 ambientLightColor;
    u8 __padding2;
    glm::vec3 directionalLightDirToSource;
    u8 __padding3;
  } portalFragUbo;

  portalFragUbo.directionalLightColor = glm::vec3(0.5f, 0.5f, 0.5f);
  portalFragUbo.ambientLightColor = glm::vec3(0.2f, 0.2f, 0.2f);
  portalFragUbo.directionalLightDirToSource = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

  // UBOs
  GLuint projViewModelUBOid;
  glGenBuffers(1, &projViewModelUBOid);
  // allocate size for buffer
  glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewModelUBO), NULL, GL_STREAM_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  // attach buffer to ubo binding point
  glBindBufferRange(GL_UNIFORM_BUFFER, projectionViewModelUBOBindingIndex, projViewModelUBOid, 0, sizeof(ProjectionViewModelUBO));

  GLuint portalFragUBOid;
  glGenBuffers(1, &portalFragUBOid);
  // allocate size for buffer
  glBindBuffer(GL_UNIFORM_BUFFER, portalFragUBOid);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(PortalFragUBO), &portalFragUbo, GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  // attach buffer to ubo binding point
  glBindBufferRange(GL_UNIFORM_BUFFER, portalFragUBOBindingIndex, portalFragUBOid, 0, sizeof(PortalFragUBO));


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
    }

    // gather input
    b32 leftShiftIsActive = isActive(KeyboardInput_Shift_Left);
    b32 leftIsActive = isActive(KeyboardInput_A) || isActive(KeyboardInput_Left);
    b32 rightIsActive = isActive(KeyboardInput_D) || isActive(KeyboardInput_Right);
    b32 upIsActive = isActive(KeyboardInput_W) || isActive(KeyboardInput_Up);
    b32 downIsActive = isActive(KeyboardInput_S) || isActive(KeyboardInput_Down);
    Vec2_f64 mouseDelta = getMouseDelta();

    // use input to modify firstPersonCamera
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
          cameraMovementDirection += rightIsActive ? firstPersonCamera.right : -firstPersonCamera.right;
        }

        if (forwardMovement)
        {
          cameraMovementDirection += upIsActive ? firstPersonCamera.forward : -firstPersonCamera.forward;
        }

        cameraMovementDirection = glm::normalize(glm::vec3(cameraMovementDirection.x, 0.0f, cameraMovementDirection.z));
        firstPersonCamera.origin += cameraMovementDirection * cameraMovementSpeed * stopWatch.delta;
      }

      rotateCamera(&firstPersonCamera, f32(-mouseDelta.y * 0.001), f32(mouseDelta.x * 0.001));
      projectionViewModelUbo.view = getViewMatrix(firstPersonCamera);
    }

    // draw
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // universal matrices in UBO
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    // TODO: We don't need to copy over projection but will have to for each portal soon
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &projectionViewModelUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // draw gate
    {
      glStencilFunc(GL_ALWAYS, // stencil function always passes
                    0x00, // reference
                    0x00); // mask

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(gateModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);

      // draw gate
      glUseProgram(gateShader.id);
      drawTriangles(gateModelVertAtt);

      glUseProgram(skyboxShader.id);
      setUniform(skyboxShader.id, "skybox", mainSkyboxTextureIndex);
      drawTriangles(invertedCubePosVertexAtt);
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

      glUseProgram(shapeShader.id);
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(portalModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);

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
      shapeModelMatrix = glm::rotate(shapeModelMatrix,
                                     30.0f * RadiansPerDegree * stopWatch.delta,
                                     glm::vec3(0.0f, 1.0f, 0.0f));

      // all shapes use the same model matrix
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(shapeModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);

      // portal 1
      {
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x01, // ref
                      0xFF); // enable which bits in reference and stored value are compared

        glUseProgram(skyboxShader.id);
        setUniform(skyboxShader.id, "skybox", portal1SkyboxTextureIndex);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(shapeShader.id);
        setUniform(shapeShader.id, "color", glm::vec3(0.4, 0.4, 1.0));
        drawTriangles(torusPentagonModelVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(shapeShader.id, "color", wireFrameColor);
        drawTriangles(torusPentagonModelVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }

      // portal 2
      {
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x02, // ref
                      0xFF); // enable which bits in reference and stored value are compared

        glUseProgram(skyboxShader.id);
        setUniform(skyboxShader.id, "skybox", portal2SkyboxTextureIndex);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(shapeShader.id);
        setUniform(shapeShader.id, "color", glm::vec3(0.4, 1.0, 0.4));
        drawTriangles(crystalModelVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(shapeShader.id, "color", wireFrameColor);
        drawTriangles(crystalModelVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }

      // portal 3
      {
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x03, // ref
                      0xFF); // enable which bits in reference and stored value are compared

        glUseProgram(skyboxShader.id);
        setUniform(skyboxShader.id, "skybox", portal3SkyboxTextureIndex);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(shapeShader.id);
        setUniform(shapeShader.id, "color", glm::vec3(0.9, 0.9, 0.9));
        drawTriangles(icosphereModelVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(shapeShader.id, "color", wireFrameColor);
        drawTriangles(icosphereModelVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }

      // portal 4
      {
        glStencilFunc(GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                      0x04, // ref
                      0xFF); // enable which bits in reference and stored value are compared

        glUseProgram(skyboxShader.id);
        setUniform(skyboxShader.id, "skybox", portal4SkyboxTextureIndex);
        drawTriangles(invertedCubePosVertexAtt);

        // draw cube
        glUseProgram(shapeShader.id);
        setUniform(shapeShader.id, "color", glm::vec3(1.0, 0.4, 0.4));
        drawTriangles(pyramidVertAtt);

        // draw wireframe
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        setUniform(shapeShader.id, "color", wireFrameColor);
        drawTriangles(pyramidVertAtt);
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
      }
    }

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(shapeShader);
  deleteShaderProgram(skyboxShader);
  deleteVertexAtts(ArrayCount(modelPtrs), modelPtrs);
}