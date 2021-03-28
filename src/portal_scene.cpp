#include "vertex_attributes.h"
#include "shader_program.h"
#include "file_locations.h"
#include "glfw_util.cpp"
#include "util.h"
#include "textures.h"
#include "camera.h"
#include "uniform_buffer_object_structs.h"
#include "model.h"
#include "noop_3d_math.h"

const Vec3 defaultPlayerDimensionInMeters = Vec3(0.5f, 0.25f, 1.75f); // NOTE: ~1'7"w, 9"d, 6'h

struct BoundingBox {
  Vec3 min;
  Vec3 dimensionInMeters;
};

struct Player {
  BoundingBox boundingBox;
};

inline Vec3 calcBoundingBoxCenterPosition(BoundingBox box) {
  return box.min + (box.dimensionInMeters * 0.5f);
}

// assume "eyes" (FPS camera) is positioned on middle X, max Y, max Z
inline Vec3 calcPlayerViewingPosition(Player* player) {
  return player->boundingBox.min + (player->boundingBox.dimensionInMeters * Vec3(0.5f, 1.0f, 1.0f));
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
  player.boundingBox.dimensionInMeters = defaultPlayerDimensionInMeters;
  player.boundingBox.min = Vec3(-(player.boundingBox.dimensionInMeters.x * 0.5f), -10.0f - (player.boundingBox.dimensionInMeters.y * 0.5f), 0.0f );

  Model gateModel = loadModel(gateModelLoc);

  VertexAtt tetrahedronVertAtt, octahedronModelVertAtt, cubeModelVertAtt, icosahedronModelVertAtt;
  VertexAtt* shapeModelPtrs[] = {&tetrahedronVertAtt, &octahedronModelVertAtt, &cubeModelVertAtt, &icosahedronModelVertAtt };
  const char* shapeModelLocs[] = { tetrahedronModelLoc, octahedronModelLoc, cubeModelLoc, icosahedronModelLoc };
  loadModelsVertexAtt(shapeModelLocs, shapeModelPtrs, ArrayCount(shapeModelPtrs));

  const f32 portalScale = 3.0f;
  const Vec3 portalPosition = Vec3(0.0f, 0.0f, portalScale * 0.5f);

  const f32 gateScale = portalScale;
  const Vec3 gatePosition = portalPosition;

  const f32 shapeScale = 1.5f;
  const Vec3 shapePosition = portalPosition;
  Vec3 shapesWireFrameColor = Vec3(0.1f, 0.1f, 0.1f);

  Vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&player);
  Vec3 firstPersonCameraInitFocus = Vec3(gatePosition.x, gatePosition.y, firstPersonCameraInitPosition.z);
  Camera camera = lookAt_FirstPerson(firstPersonCameraInitPosition, firstPersonCameraInitFocus);

  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  VertexAtt invertedCubePosVertexAtt = initializeCubePositionVertexAttBuffers(true);

  Mat4 shapeModelMatrix = glm::translate(mat4_identity, shapePosition) * Mat4(Mat3(shapeScale));
  Mat4 portalModelMatrix = glm::translate(mat4_identity, portalPosition) * Mat4(Mat3(portalScale));
  Mat4 gateModelMatrix = glm::translate(mat4_identity, gatePosition) * Mat4(Mat3(gateScale));
  Mat4 playerScaleModelMatrix = glm::scale(mat4_identity, player.boundingBox.dimensionInMeters);
  const f32 originalProjectionDepthNear = 0.1f;
  const f32 originalProjectionDepthFar = 200.0f;
  projectionViewModelUbo.projection = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, originalProjectionDepthNear, originalProjectionDepthFar);

  GLuint mainSkyboxTextureId, calmSeaSkyboxTextureId, interstellarSkyboxTextureId, pollutedEarthSkyboxTextureId, yellowCloudSkyboxTextureId;
  loadCubeMapTexture(caveFaceLocations, &mainSkyboxTextureId);
  loadCubeMapTexture(calmSeaFaceLocations, &calmSeaSkyboxTextureId);
  loadCubeMapTexture(skyboxInterstellarFaceLocations, &interstellarSkyboxTextureId);
  loadCubeMapTexture(pollutedEarthFaceLocations , &pollutedEarthSkyboxTextureId);
  loadCubeMapTexture(yellowCloudFaceLocations, &yellowCloudSkyboxTextureId);

  s32 mainSkyboxTextureIndex = 0;
  s32 portalNegativeXSkyboxTextureIndex = 1;
  s32 portalPositiveXSkyboxTextureIndex = 2;
  s32 portalNegativeYSkyboxTextureIndex = 3;
  s32 portalPositiveYSkyboxTextureIndex = 4;
  s32 modelAlbedoTextureIndex = 5;
  s32 modelNormalTextureIndex = 6;
  bindActiveTextureCubeMap(mainSkyboxTextureIndex, mainSkyboxTextureId);
  bindActiveTextureCubeMap(portalNegativeXSkyboxTextureIndex, calmSeaSkyboxTextureId);
  bindActiveTextureCubeMap(portalPositiveXSkyboxTextureIndex, interstellarSkyboxTextureId);
  bindActiveTextureCubeMap(portalNegativeYSkyboxTextureIndex, yellowCloudSkyboxTextureId);
  bindActiveTextureCubeMap(portalPositiveYSkyboxTextureIndex, pollutedEarthSkyboxTextureId);
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
    Vec3 directionalLightColor;
    u8 __padding1;
    Vec3 ambientLightColor;
    u8 __padding2;
    Vec3 directionalLightDirToSource;
    u8 __padding3;
  } portalFragUbo;

  portalFragUbo.directionalLightColor = Vec3(0.5f, 0.5f, 0.5f);
  portalFragUbo.ambientLightColor = Vec3(0.2f, 0.2f, 0.2f);
  portalFragUbo.directionalLightDirToSource = Vec3(1.0f, 1.0f, 1.0f);

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
  const u32 portalNegativeYStencilMask = 0x03;
  const u32 portalPositiveYStencilMask = 0x04;

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

  Vec3 portalNegativeXCenter = cubeFaceNegativeXCenter * portalScale;
  Vec3 portalPositiveXCenter = cubeFacePositiveXCenter * portalScale;
  Vec3 portalNegativeYCenter = cubeFaceNegativeYCenter * portalScale;
  Vec3 portalPositiveYCenter = cubeFacePositiveYCenter * portalScale;

  b32 cameraIsThirdPerson = false;
  StopWatch stopWatch = createStopWatch();
  while(glfwWindowShouldClose(window) == GL_FALSE)
  {
    loadInputStateForFrame(window);
    updateStopWatch(&stopWatch);

    f32 sinTime = sin(stopWatch.lastFrame);
    f32 cosTime = cos(stopWatch.lastFrame);

    Vec3 playerCenter = calcBoundingBoxCenterPosition(player.boundingBox);
    Vec3 playerViewPosition = calcPlayerViewingPosition(&player);

    if (isActive(KeyboardInput_Esc))
    {
      glfwSetWindowShouldClose(window, true);
      break;
    }

    // toggle fullscreen/window mode if alt + enter
    if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
      windowExtent = toggleWindowSize(window, initWindowExtent.width, initWindowExtent.height);
      aspectRatio = f32(windowExtent.width) / windowExtent.height;
      Mat4 projectionMatrix = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, 0.1f, 100.0f);
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
      Vec3 cameraDelta{};
      if (lateralMovement || forwardMovement)
      {
        f32 cameraMovementSpeed = leftShiftIsActive ? 5.0f : 1.0f;

        // Camera movement direction
        Vec3 cameraMovementDirection{};
        if (lateralMovement)
        {
          cameraMovementDirection += rightIsActive ? camera.right : -camera.right;
        }

        if (forwardMovement)
        {
          cameraMovementDirection += upIsActive ? camera.forward : -camera.forward;
        }

        cameraMovementDirection = glm::normalize(Vec3(cameraMovementDirection.x, cameraMovementDirection.y, 0.0));
        cameraDelta = cameraMovementDirection * cameraMovementSpeed * stopWatch.delta;
      }

      if(tabHotPress) {
        cameraIsThirdPerson = !cameraIsThirdPerson;
        Vec3 xyForward = glm::normalize(Vec3(gatePosition.x - playerCenter.x, gatePosition.y - playerCenter.y, 0.0f));

        if(cameraIsThirdPerson) {
          camera = lookAt_ThirdPerson(playerCenter, xyForward);
        } else { // camera is first person now
          Vec3 focus = playerViewPosition + xyForward;
          camera = lookAt_FirstPerson(playerViewPosition, camera.forward);
        }
      }

      const f32 mouseDeltaMultConst = 0.001f;
      if(cameraIsThirdPerson) {
        updateCamera_ThirdPerson(&camera, playerCenter, f32(-mouseDelta.y * mouseDeltaMultConst),f32(-mouseDelta.x * 0.001f));
      } else {
        updateCamera_FirstPerson(&camera, cameraDelta, f32(-mouseDelta.y * mouseDeltaMultConst),f32(-mouseDelta.x * 0.001f));
      }
      player.boundingBox.min += cameraDelta;

      projectionViewModelUbo.view = getViewMatrix(camera);
    }

    // draw
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // universal matrices in UBO
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    // TODO: don't need to set projection matrix right now but will in the future so keeping
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &projectionViewModelUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    b32 gateIsInFront = glm::dot(camera.forward, glm::normalize(gatePosition - camera.origin)) > 0;
    b32 portalNegativeXVisible = (glm::dot(camera.origin - portalNegativeXCenter, cubeFaceNegativeXNormal) > 0.0f) && gateIsInFront;
    b32 portalPositiveXVisible = (glm::dot(camera.origin - portalPositiveXCenter, cubeFacePositiveXNormal) > 0.0f) && gateIsInFront;
    b32 portalNegativeYVisible = (glm::dot(camera.origin - portalNegativeYCenter, cubeFaceNegativeYNormal) > 0.0f) && gateIsInFront;
    b32 portalPositiveYVisible = (glm::dot(camera.origin - portalPositiveYCenter, cubeFacePositiveYNormal) > 0.0f) && gateIsInFront;

    glStencilFunc(GL_ALWAYS, // stencil function always passes
                  0x00, // reference
                  0x00); // mask

    glUseProgram(skyboxShader.id);
    setUniform(skyboxShader.id, "skybox", mainSkyboxTextureIndex);
    drawTriangles(invertedCubePosVertexAtt);

    if(cameraIsThirdPerson) { // draw player if third person
      Vec3 playerCenter = calcBoundingBoxCenterPosition(player.boundingBox);
      Vec3 playerViewCenter = calcPlayerViewingPosition(&player);
      Vec3 playerBoundingBoxColor_Red = Vec3(1.0f, 0.0f, 0.0f);
      Vec3 playerViewBoxColor_White = Vec3(1.0f, 1.0f, 1.0f);
      Vec3 playerMinCoordBoxColor_Green = Vec3(0.0f, 1.0f, 0.0f);

      Mat4 thirdPersonPlayerBoxesMatrix;

      glUseProgram(shapeShader.id);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesMatrix = glm::translate(mat4_identity, playerCenter) * playerScaleModelMatrix;
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "color", playerBoundingBoxColor_Red);
      drawTriangles(cubePosVertexAtt);

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesMatrix = glm::translate(mat4_identity, player.boundingBox.min) * Mat4(Mat3(0.1f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "color", playerMinCoordBoxColor_Green);
      drawTriangles(cubePosVertexAtt);

      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesMatrix = glm::translate(mat4_identity, playerViewCenter) * Mat4(Mat3(0.1f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesMatrix));
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
        portalFragUbo.directionalLightDirToSource = glm::normalize(Vec3(cosTime, sinTime, cosTime)); // orbit xyplane (ground), oscillate up and down
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(PortalFragUBO, directionalLightDirToSource), sizeof(Vec4), &portalFragUbo.directionalLightDirToSource);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(gateModelMatrix));
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
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(portalModelMatrix));
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

        if (portalNegativeYVisible)
        {
          glStencilMask(portalNegativeYStencilMask);
          drawTriangles(cubePosVertexAtt, 6, cubeFaceNegativeYIndicesOffset);
        }

        if (portalPositiveYVisible)
        {
          glStencilMask(portalPositiveYStencilMask);
          drawTriangles(cubePosVertexAtt, 6, cubeFacePositiveYIndicesOffset);
        }
      }

      // turn off writes to the stencil
      glStencilMask(0x00);

      // We need to clear disable depth values so distant objects through the "portals" still get drawn
      // The portals themselves will still obey the depth of the scene, as the stencils have been rendered with depth in mind
      glClear(GL_DEPTH_BUFFER_BIT);
      { // use stencils to draw portals
        shapeModelMatrix = glm::rotate(shapeModelMatrix,30.0f * RadiansPerDegree * stopWatch.delta, Vec3(0.0f, 0.0f, 1.0f));

        // all shapes use the same model matrix
        glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(shapeModelMatrix));
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
          setUniform(shapeShader.id, "color", Vec3(0.4, 0.4, 1.0));
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
          setUniform(shapeShader.id, "color", Vec3(0.4, 1.0, 0.4));
          drawTriangles(octahedronModelVertAtt);

          // draw wireframe
          drawShapeWireframe(&octahedronModelVertAtt);
        }

        // portal negative y
        if (portalNegativeYVisible)
        {
          glStencilFunc(
                  GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                  portalNegativeYStencilMask, // ref
                  0xFF); // enable which bits in reference and stored value are compared

          glUseProgram(skyboxShader.id);
          setUniform(skyboxShader.id, "skybox", portalNegativeYSkyboxTextureIndex);
          drawTriangles(invertedCubePosVertexAtt);

          // draw cube
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "color", Vec3(1.0, 0.4, 0.4));
          drawTriangles(tetrahedronVertAtt);

          // draw wireframe
          drawShapeWireframe(&tetrahedronVertAtt);
        }

        // portal positive y
        if (portalPositiveYVisible)
        {
          glStencilFunc(
                  GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
                  portalPositiveYStencilMask, // ref
                  0xFF); // enable which bits in reference and stored value are compared

          glUseProgram(skyboxShader.id);
          setUniform(skyboxShader.id, "skybox", portalPositiveYSkyboxTextureIndex);
          drawTriangles(invertedCubePosVertexAtt);

          // draw cube
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "color", Vec3(0.9, 0.9, 0.9));
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