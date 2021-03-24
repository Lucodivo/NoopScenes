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
const glm::mat4 mat4_identity = glm::mat4(1.0f);

struct Player {
  glm::vec3 minBoxPosition;
  glm::vec3 dimensionInMeters;
  // assume "eyes" (FPS camera) is positioned on front Z, top Y, middle X
};

inline glm::vec3 calcPlayerCenterPosition(Player* player) {
  return player->minBoxPosition + (player->dimensionInMeters * 0.5f);
}

inline glm::vec3 calcPlayerViewingPosition(Player* player) {
  return player->minBoxPosition + (player->dimensionInMeters * glm::vec3(0.5f, 1.0f, 1.0f));
}

void changeNearFarProjection(glm::mat4* projectionMatrix, f32 zNear, f32 zFar) {
  // Note: logic pulled straight from glm::perspective -> perspectiveRH_NO
  (*projectionMatrix)[2][2] = - (zFar + zNear) / (zFar - zNear);
  (*projectionMatrix)[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
}

void portalScene(GLFWwindow* window) {
  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  ShaderProgram gateShader = createShaderProgram(gateVertexShaderFileLoc, gateFragmentShaderFileLoc);
  ShaderProgram shapeShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);

  ProjectionViewModelUBO projectionViewModelUbo;
  Player player;
  player.dimensionInMeters = defaultPlayerDimensionInMeters;
  player.minBoxPosition = glm::vec3(-(player.dimensionInMeters.x * 0.5f), 0.0f, -10.0f - (player.dimensionInMeters.z * 0.5f));

  Model gateModel = loadModel(gateModelLoc);

  VertexAtt tetrahedronVertAtt, octahedronModelVertAtt, cubeModelVertAtt, icosahedronModelVertAtt;
  VertexAtt* shapeModelPtrs[] = {&tetrahedronVertAtt, &octahedronModelVertAtt, &cubeModelVertAtt, &icosahedronModelVertAtt };
  const char* shapeModelLocs[] = { tetrahedronModelLoc, octahedronModelLoc, cubeModelLoc, icosahedronModelLoc };
  loadModelsVertexAtt(shapeModelLocs, shapeModelPtrs, ArrayCount(shapeModelPtrs));

  const f32 portalScale = 3.0f;
  const glm::vec3 portalPosition = glm::vec3(0.0f, portalScale * 0.5f, 0.0);

  const f32 gateScale = portalScale;
  const glm::vec3 gatePosition = portalPosition;

  const f32 shapeScale = 1.5f;
  const glm::vec3 shapePosition = portalPosition;
  glm::vec3 shapesWireFrameColor = glm::vec3(0.1f, 0.1f, 0.1f);

  glm::vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&player);
  glm::vec3 firstPersonCameraInitFocus = glm::vec3(gatePosition.x, firstPersonCameraInitPosition.y, gatePosition.z);
  Camera camera = lookAt(firstPersonCameraInitPosition, firstPersonCameraInitFocus);

  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  VertexAtt invertedCubePosVertexAtt = initializeCubePositionVertexAttBuffers(true);

  glm::mat4 shapeModelMatrix = glm::translate(mat4_identity, shapePosition) * glm::mat4(glm::mat3(shapeScale));
  glm::mat4 portalModelMatrix = glm::translate(mat4_identity, portalPosition) * glm::mat4(glm::mat3(portalScale));
  glm::mat4 gateModelMatrix = glm::translate(mat4_identity, gatePosition) * glm::mat4(glm::mat3(gateScale));
  glm::mat4 playerScaleModelMatrix = glm::scale(mat4_identity, player.dimensionInMeters);
  const f32 originalProjectionZNear = 0.1f;
  const f32 originalProjectionZFar = 200.0f;
  projectionViewModelUbo.projection = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, originalProjectionZNear, originalProjectionZFar);

  GLuint mainSkyboxTextureId, calmSeaSkyboxTextureId, interstellarSkyboxTextureId, pollutedEarthSkyboxTextureId, yellowCloudSkyboxTextureId;
  loadCubeMapTexture(caveFaceLocations, &mainSkyboxTextureId);
  loadCubeMapTexture(calmSeaFaceLocations, &calmSeaSkyboxTextureId);
  loadCubeMapTexture(skyboxInterstellarFaceLocations, &interstellarSkyboxTextureId);
  loadCubeMapTexture(pollutedEarthFaceLocations , &pollutedEarthSkyboxTextureId);
  loadCubeMapTexture(yellowCloudFaceLocations, &yellowCloudSkyboxTextureId);

  s32 mainSkyboxTextureIndex = 0;
  s32 portalNegativeXSkyboxTextureIndex = 1;
  s32 portalPositiveXSkyboxTextureIndex = 2;
  s32 portalNegativeZSkyboxTextureIndex = 3;
  s32 portalPositiveZSkyboxTextureIndex = 4;
  s32 modelAlbedoTextureIndex = 5;
  s32 modelNormalTextureIndex = 6;
  bindActiveTextureCubeMap(mainSkyboxTextureIndex, mainSkyboxTextureId);
  bindActiveTextureCubeMap(portalNegativeXSkyboxTextureIndex, calmSeaSkyboxTextureId);
  bindActiveTextureCubeMap(portalPositiveXSkyboxTextureIndex, interstellarSkyboxTextureId);
  bindActiveTextureCubeMap(portalNegativeZSkyboxTextureIndex, yellowCloudSkyboxTextureId);
  bindActiveTextureCubeMap(portalPositiveZSkyboxTextureIndex, pollutedEarthSkyboxTextureId);
  bindActiveTextureSampler2d(modelAlbedoTextureIndex, gateModel.textureData.albedoTextureId);
  bindActiveTextureSampler2d(modelNormalTextureIndex, gateModel.textureData.normalTextureId);

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
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
  portalFragUbo.directionalLightDirToSource = glm::vec3(1.0f, 1.0f, 1.0f);

  // UBOs
  GLuint projViewModelUBOid, portalFragUBOid;
  {
    glGenBuffers(1, &projViewModelUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewModelUBO), NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, projectionViewModelUBOBindingIndex, projViewModelUBOid, 0, sizeof(ProjectionViewModelUBO));

    glGenBuffers(1, &portalFragUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, portalFragUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PortalFragUBO), &portalFragUbo, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, portalFragUBOBindingIndex, portalFragUBOid, 0, sizeof(PortalFragUBO));
  }

  const u32 portalNegativeXStencilMask = 0x01;
  const u32 portalPositiveXStencilMask = 0x02;
  const u32 portalNegativeZStencilMask = 0x03;
  const u32 portalPositiveZStencilMask = 0x04;

  auto drawShapeWireframe = [shapeShader, shapesWireFrameColor](VertexAtt* vertAtt) -> void {
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);
    setUniform(shapeShader.id, "color", shapesWireFrameColor);
    drawTriangles(*vertAtt);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
  };

  glm::vec3 portalNegativeXCenter = cubeFaceNegativeXCenter * portalScale;
  glm::vec3 portalPositiveXCenter = cubeFacePositiveXCenter * portalScale;
  glm::vec3 portalNegativeZCenter = cubeFaceNegativeZCenter * portalScale;
  glm::vec3 portalPositiveZCenter = cubeFacePositiveZCenter * portalScale;

  b32 cameraIsThirdPerson = false;
  StopWatch stopWatch = createStopWatch();
  while(glfwWindowShouldClose(window) == GL_FALSE)
  {
    loadInputStateForFrame(window);
    updateStopWatch(&stopWatch);

    f32 sinTime = sin(stopWatch.lastFrame);
    f32 cosTime = cos(stopWatch.lastFrame);

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
    b32 tabHotPress = hotPress(KeyboardInput_Tab);
    Vec2_f64 mouseDelta = getMouseDelta();

    // use input to modify camera
    {
      b32 lateralMovement = leftIsActive != rightIsActive;
      b32 forwardMovement = upIsActive != downIsActive;
      glm::vec3 cameraDelta{};
      if (lateralMovement || forwardMovement)
      {
        f32 cameraMovementSpeed = leftShiftIsActive ? 5.0f : 1.0f;

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
        cameraDelta = cameraMovementDirection * cameraMovementSpeed * stopWatch.delta;
      }

      if(tabHotPress) {
        cameraIsThirdPerson = !cameraIsThirdPerson;
        glm::vec3 xzForward = glm::normalize(glm::vec3(camera.forward.x, 0.0f, camera.forward.z));
        glm::vec3 newCameraOrigin, newCameraLookAtPos;

        if(cameraIsThirdPerson) {
          newCameraLookAtPos = calcPlayerCenterPosition(&player);
          newCameraOrigin = newCameraLookAtPos - (8.0f * xzForward); // move back
          newCameraOrigin.y += 4.0f; // move up
        } else { // camera is first person now
          newCameraOrigin = calcPlayerViewingPosition(&player);
          newCameraLookAtPos = newCameraOrigin + xzForward;
        }
        camera = lookAt(newCameraOrigin, newCameraLookAtPos);
      }

      updateCameraFirstPerson(&camera, cameraDelta, f32(-mouseDelta.y * 0.001), f32(mouseDelta.x * 0.001));

      projectionViewModelUbo.view = getViewMatrix(camera);
    }

    // draw
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // universal matrices in UBO
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &projectionViewModelUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    b32 gateIsInFront = glm::dot(camera.forward, glm::normalize(gatePosition - camera.origin)) > 0;
    glm::vec3 negativeViewDir = -camera.forward;
    b32 portalNegativeXVisible = (glm::dot(camera.origin - portalNegativeXCenter, cubeFaceNegativeXNormal) > 0.0f) && gateIsInFront;
    b32 portalPositiveXVisible = (glm::dot(camera.origin - portalPositiveXCenter, cubeFacePositiveXNormal) > 0.0f) && gateIsInFront;
    b32 portalNegativeZVisible = (glm::dot(camera.origin - portalNegativeZCenter, cubeFaceNegativeZNormal) > 0.0f) && gateIsInFront;
    b32 portalPositiveZVisible = (glm::dot(camera.origin - portalPositiveZCenter, cubeFacePositiveZNormal) > 0.0f) && gateIsInFront;

    glStencilFunc(GL_ALWAYS, // stencil function always passes
                  0x00, // reference
                  0x00); // mask

    glUseProgram(skyboxShader.id);
    setUniform(skyboxShader.id, "skybox", mainSkyboxTextureIndex);
    drawTriangles(invertedCubePosVertexAtt);

    if(cameraIsThirdPerson) { // draw player if third person
      glm::vec3 playerCenter = calcPlayerCenterPosition(&player);
      glm::vec3 playerViewCenter = calcPlayerViewingPosition(&player);
      glm::vec3 playerBoundingBoxColor_Red = glm::vec3(1.0f, 0.0f, 0.0f);
      glm::vec3 playerViewBoxColor_White = glm::vec3(1.0f, 1.0f, 1.0f);
      glm::vec3 playerMinCoordBoxColor_Green = glm::vec3(0.0f, 1.0f, 0.0f);

      glm::mat4 thirdPersonPlayerBoxesMatrix;

      glUseProgram(shapeShader.id);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesMatrix = glm::translate(mat4_identity, playerCenter) * playerScaleModelMatrix;
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(thirdPersonPlayerBoxesMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "color", playerBoundingBoxColor_Red);
      drawTriangles(cubePosVertexAtt);

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesMatrix = glm::translate(mat4_identity, player.minBoxPosition) * glm::mat4(glm::mat3(0.1f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(thirdPersonPlayerBoxesMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "color", playerMinCoordBoxColor_Green);
      drawTriangles(cubePosVertexAtt);

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesMatrix = glm::translate(mat4_identity, playerViewCenter) * glm::mat4(glm::mat3(0.1f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(thirdPersonPlayerBoxesMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "color", playerViewBoxColor_White);
      drawTriangles(cubePosVertexAtt);

      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if(gateIsInFront)
    {
      { // draw gate
        glBindBuffer(GL_UNIFORM_BUFFER, portalFragUBOid);
        portalFragUbo.directionalLightDirToSource = glm::vec3(sinTime, 1.0f, cosTime);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(PortalFragUBO, directionalLightDirToSource), sizeof(glm::vec4),
                        &portalFragUbo.directionalLightDirToSource);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4), glm::value_ptr(gateModelMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glUseProgram(gateShader.id);
        setUniform(gateShader.id, "albedoTex", modelAlbedoTextureIndex);
        setUniform(gateShader.id, "normalTex", modelNormalTextureIndex);
        drawTriangles(gateModel.vertexAtt);
      }

      { // draw out portal stencils
        // NOTE: Stencil function Example
        // GL_LEQUAL
        // Passes if ( ref & mask ) <= ( stencil & mask )
        glStencilFunc(GL_EQUAL, // func
                      0xFF, // ref
                      0x00); // mask // Only draw portals where the stencil is cleared
        glStencilOp(GL_KEEP, // action when stencil fails
                    GL_KEEP, // action when stencil passes but depth fails
                    GL_REPLACE); // action when both stencil and depth pass

        glUseProgram(shapeShader.id);
        glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(glm::mat4),
                        glm::value_ptr(portalModelMatrix));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        if (portalNegativeXVisible)
        {
          glStencilMask(portalNegativeXStencilMask);
          drawTriangles(cubePosVertexAtt, 6, cubeFaceNegativeXIndicesOffset);
        }

        if (portalPositiveXVisible)
        {
          glStencilMask(portalPositiveXStencilMask);
          drawTriangles(cubePosVertexAtt, 6, cubeFacePositiveXIndicesOffset);
        }

        if (portalNegativeZVisible)
        {
          glStencilMask(portalNegativeZStencilMask);
          drawTriangles(cubePosVertexAtt, 6, cubeFaceNegativeZIndicesOffset);
        }

        if (portalPositiveZVisible)
        {
          glStencilMask(portalPositiveZStencilMask);
          drawTriangles(cubePosVertexAtt, 6, cubeFacePositiveZIndicesOffset);
        }
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

        // portal negative x
        if (portalNegativeXVisible)
        {
          glStencilFunc(
                  GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                  portalNegativeXStencilMask, // ref
                  0xFF); // enable which bits in reference and stored value are compared

          glUseProgram(skyboxShader.id);
          setUniform(skyboxShader.id, "skybox", portalNegativeXSkyboxTextureIndex);
          drawTriangles(invertedCubePosVertexAtt);

          // draw cube
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "color", glm::vec3(0.4, 0.4, 1.0));
          drawTriangles(cubeModelVertAtt);

          // draw wireframe
          drawShapeWireframe(&cubeModelVertAtt);
        }

        // portal positive x
        if (portalPositiveXVisible)
        {
          glStencilFunc(
                  GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                  portalPositiveXStencilMask, // ref
                  0xFF); // enable which bits in reference and stored value are compared

          glUseProgram(skyboxShader.id);
          setUniform(skyboxShader.id, "skybox", portalPositiveXSkyboxTextureIndex);
          drawTriangles(invertedCubePosVertexAtt);

          // draw cube
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "color", glm::vec3(0.4, 1.0, 0.4));
          drawTriangles(octahedronModelVertAtt);

          // draw wireframe
          drawShapeWireframe(&octahedronModelVertAtt);
        }

        // portal negative z
        if (portalNegativeZVisible)
        {
          glStencilFunc(
                  GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                  portalNegativeZStencilMask, // ref
                  0xFF); // enable which bits in reference and stored value are compared

          glUseProgram(skyboxShader.id);
          setUniform(skyboxShader.id, "skybox", portalNegativeZSkyboxTextureIndex);
          drawTriangles(invertedCubePosVertexAtt);

          // draw cube
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "color", glm::vec3(1.0, 0.4, 0.4));
          drawTriangles(tetrahedronVertAtt);

          // draw wireframe
          drawShapeWireframe(&tetrahedronVertAtt);
        }

        // portal positive z
        if (portalPositiveZVisible)
        {
          glStencilFunc(
                  GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                  portalPositiveZStencilMask, // ref
                  0xFF); // enable which bits in reference and stored value are compared

          glUseProgram(skyboxShader.id);
          setUniform(skyboxShader.id, "skybox", portalPositiveZSkyboxTextureIndex);
          drawTriangles(invertedCubePosVertexAtt);

          // draw cube
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "color", glm::vec3(0.9, 0.9, 0.9));
          drawTriangles(icosahedronModelVertAtt);

          // draw wireframe
          drawShapeWireframe(&icosahedronModelVertAtt);
        }
      }
    }

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(gateShader);
  deleteShaderProgram(shapeShader);
  deleteShaderProgram(skyboxShader);
  deleteVertexAtts(ArrayCount(shapeModelPtrs), shapeModelPtrs);
  deleteVertexAtt(gateModel.vertexAtt);
}