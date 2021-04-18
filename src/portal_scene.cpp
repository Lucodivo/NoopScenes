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

#define NO_STENCIL_MASK 0

const Vec3 defaultPlayerDimensionInMeters = Vec3(0.5f, 0.25f, 1.75f); // NOTE: ~1'7"w, 9"d, 6'h

struct Player {
  BoundingBox boundingBox;
};

enum CubeSide {
  CubeSide_NegativeX,
  CubeSide_PositiveX,
  CubeSide_NegativeY,
  CubeSide_PositiveY,
};

enum SceneState {
  SceneState_Gate,
  SceneState_1,
  SceneState_2,
  SceneState_3,
  SceneState_4
};

inline Vec3 calcBoundingBoxCenterPosition(BoundingBox box) {
  return box.min + (box.dimensionInMeters * 0.5f);
}

// assume "eyes" (FPS camera) is positioned on middle X, max Y, max Z
inline Vec3 calcPlayerViewingPosition(Player* player) {
  return player->boundingBox.min + (player->boundingBox.dimensionInMeters * Vec3(0.5f, 1.0f, 1.0f));
}

void drawModelWireframe(const Model& model) {
  glDisable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_CULL_FACE);
  drawModel(model);
  glEnable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);
}

struct Scene
{
  ShaderProgram* skyboxShader;
  GLuint environmentTextureIndex;
  VertexAtt environmentBoxVertexAtt;
  ShaderProgram* modelShader;
  VertexAtt* modelVertexAtts;
  u32 modelCount;
  VertexAtt* wireframeModelVertexAtt;
  u32 wireframeModelCount;
};

struct Entity {
  ShaderProgram* shaderProgram;
  VertexAtt* vertexAtt;
  Vec3 position;
};

struct World
{
  Camera camera;
  Player player;
  SceneState scene;
  std::vector<Entity> entities;
};

bool insideBox(BoundingBox boundingBox, Vec3 position) {
  const Vec3 boundingBoxMax = boundingBox.min + boundingBox.dimensionInMeters;
  return (position.x > boundingBox.min.x &&
          position.x < boundingBoxMax.x &&
          position.y > boundingBox.min.y &&
          position.y < boundingBoxMax.y &&
          position.z > boundingBox.min.z &&
          position.z < boundingBoxMax.z);
}


void portalScene(GLFWwindow* window) {
  Extent2D windowExtent = getWindowExtent();
  const Extent2D initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  World world{};
  Scene gateScene{};
  Scene scene1{};
  Scene scene2{};
  Scene scene3{};
  Scene scene4{};

  ShaderProgram gateShader = createShaderProgram(gateVertexShaderFileLoc, gateFragmentShaderFileLoc);
  gateScene.modelShader = &gateShader;

  ShaderProgram shapeShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  scene1.skyboxShader = &shapeShader;
  scene2.skyboxShader = &shapeShader;
  scene3.skyboxShader = &shapeShader;
  scene4.skyboxShader = &shapeShader;

  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);
  gateScene.skyboxShader = &skyboxShader;
  scene1.skyboxShader = &skyboxShader;
  scene2.skyboxShader = &skyboxShader;
  scene3.skyboxShader = &skyboxShader;
  scene4.skyboxShader = &skyboxShader;

  ProjectionViewModelUBO projectionViewModelUbo;

  const f32 portalScale = 3.0f;
  const Vec3 portalPosition = Vec3(0.0f, 0.0f, portalScale * 0.5f);
  BoundingBox portalBoundingBox = {(cubeVertAttBoundingBox.min * portalScale) + portalPosition, cubeVertAttBoundingBox.dimensionInMeters * portalScale };

  const f32 gateScale = portalScale;
  const Vec3 gatePosition = portalPosition;

  const f32 shapeScale = 1.0f;
  const Vec3 shapePosition = portalPosition;
  Vec3 shapesWireFrameColor = Vec3(0.1f, 0.1f, 0.1f);

  Model gateModel, tetrahedronModel, octahedronModel, cubeModel, icosahedronModel;
  Model* modelPtrs[] = { &gateModel, &tetrahedronModel, &octahedronModel, &cubeModel, &icosahedronModel };
  const char* modelLocs[] = { gateModelLoc, tetrahedronModelLoc, octahedronModelLoc, cubeModelLoc, icosahedronModelLoc };
  loadModels(modelLocs, ArrayCount(modelPtrs), modelPtrs);

  gateModel.boundingBox.min *= gateScale;
  gateModel.boundingBox.min += gatePosition;
  gateModel.boundingBox.dimensionInMeters *= gateScale;

  Player player;
  player.boundingBox.dimensionInMeters = defaultPlayerDimensionInMeters;
  player.boundingBox.min = Vec3(-(player.boundingBox.dimensionInMeters.x * 0.5f), -10.0f - (player.boundingBox.dimensionInMeters.y * 0.5f), 0.0f );

  Vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&player);
  Vec3 firstPersonCameraInitFocus = Vec3(gatePosition.x, gatePosition.y, firstPersonCameraInitPosition.z);
  Camera camera;
  lookAt_FirstPerson(firstPersonCameraInitPosition, firstPersonCameraInitFocus, &camera);

  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  VertexAtt invertedCubePosVertexAtt = initializeCubePositionVertexAttBuffers(true);

  Mat4 shapeModelMatrix = glm::translate(mat4_identity, shapePosition) * Mat4(Mat3(shapeScale));
  Mat4 portalModelMatrix = glm::translate(mat4_identity, portalPosition) * Mat4(Mat3(portalScale));
  Mat4 gateModelMatrix = glm::translate(mat4_identity, gatePosition) * Mat4(Mat3(gateScale));
  Mat4 playerBoundingBoxScaleMatrix = glm::scale(mat4_identity, player.boundingBox.dimensionInMeters);
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
  s32 gateAlbedoTextureIndex = 5;
  s32 gateNormalTextureIndex = 6;
  bindActiveTextureCubeMap(mainSkyboxTextureIndex, mainSkyboxTextureId);
  bindActiveTextureCubeMap(portalNegativeXSkyboxTextureIndex, calmSeaSkyboxTextureId);
  bindActiveTextureCubeMap(portalPositiveXSkyboxTextureIndex, interstellarSkyboxTextureId);
  bindActiveTextureCubeMap(portalNegativeYSkyboxTextureIndex, yellowCloudSkyboxTextureId);
  bindActiveTextureCubeMap(portalPositiveYSkyboxTextureIndex, pollutedEarthSkyboxTextureId);

  Assert(gateModel.meshCount == 1) // gate model must have one and only one mesh
  bindActiveTextureSampler2d(gateAlbedoTextureIndex, gateModel.meshes[0].textureData.albedoTextureId);
  bindActiveTextureSampler2d(gateNormalTextureIndex, gateModel.meshes[0].textureData.normalTextureId);

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

  Vec3 portalNegativeXCenter = cubeFaceNegativeXCenter * portalScale;
  Vec3 portalPositiveXCenter = cubeFacePositiveXCenter * portalScale;
  Vec3 portalNegativeYCenter = cubeFaceNegativeYCenter * portalScale;
  Vec3 portalPositiveYCenter = cubeFacePositiveYCenter * portalScale;

  b32 portalInFocus = false;
  CubeSide portalOfFocus;

  SceneState whereAreWe = SceneState_Gate;

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

    // gather input for movement and camera changes
    {
      b32 lateralMovement = leftIsActive != rightIsActive;
      b32 forwardMovement = upIsActive != downIsActive;
      Vec3 playerDelta{};
      if (lateralMovement || forwardMovement)
      {
        f32 playerMovementSpeed = leftShiftIsActive ? 5.0f : 1.0f;

        // Camera movement direction
        Vec3 playerMovementDirection{};
        if (lateralMovement)
        {
          playerMovementDirection += rightIsActive ? camera.right : -camera.right;
        }

        if (forwardMovement)
        {
          playerMovementDirection += upIsActive ? camera.forward : -camera.forward;
        }

        playerMovementDirection = glm::normalize(Vec3(playerMovementDirection.x, playerMovementDirection.y, 0.0));
        playerDelta = playerMovementDirection * playerMovementSpeed * stopWatch.delta;
      }
      player.boundingBox.min += playerDelta;

      if(tabHotPress) { // switch between third and first person
        Vec3 xyForward = glm::normalize(Vec3(camera.forward.x, camera.forward.y, 0.0f));

        if(!camera.thirdPerson) {
          lookAt_ThirdPerson(playerCenter, xyForward, &camera);
        } else { // camera is first person now
          Vec3 focus = playerViewPosition + xyForward;
          lookAt_FirstPerson(playerViewPosition, focus, &camera);
        }
      }

      const f32 mouseDeltaMultConst = 0.001f;
      if(camera.thirdPerson) {
        updateCamera_ThirdPerson(&camera, playerCenter, f32(-mouseDelta.y * mouseDeltaMultConst),f32(-mouseDelta.x * 0.001f));
      } else {
        updateCamera_FirstPerson(&camera, playerDelta, f32(-mouseDelta.y * mouseDeltaMultConst), f32(-mouseDelta.x * 0.001f));
      }

      projectionViewModelUbo.view = getViewMat(camera);
    }

    // draw
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // universal matrices in UBO
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    // TODO: don't need to set projection matrix right now but will in the future so keeping
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &projectionViewModelUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // TODO: This should ideally relate to the player view and direction, so a third-person camera can more easily act as a debug camera
    b32 gateIsInFront = glm::dot(camera.forward, glm::normalize(gatePosition - camera.origin)) > 0;
    b32 insideGate = insideBox(gateModel.boundingBox, playerViewPosition);
    b32 gateIsVisible = gateIsInFront || insideGate;

    Vec3 portalNegativeXCenterToPlayerView = playerViewPosition - portalNegativeXCenter;
    Vec3 portalPositiveXCenterToPlayerView = playerViewPosition - portalPositiveXCenter;
    Vec3 portalNegativeYCenterToPlayerView = playerViewPosition - portalNegativeYCenter;
    Vec3 portalPositiveYCenterToPlayerView = playerViewPosition - portalPositiveYCenter;
    f32 distSquaredPortalNegativeXCenterToPlayerView = distSquared(portalNegativeXCenterToPlayerView);
    f32 distSquaredPortalPositiveXCenterToPlayerView = distSquared(portalPositiveXCenterToPlayerView);
    f32 distSquaredPortalNegativeYCenterToPlayerView = distSquared(portalNegativeYCenterToPlayerView);
    f32 distSquaredPortalPositiveYCenterToPlayerView = distSquared(portalPositiveYCenterToPlayerView);

    if(portalInFocus != insideGate) { // If transitioning between inside and outside of gate boundaries
      if(insideGate) { // transitioning to inside
        portalInFocus = true;

        portalOfFocus = CubeSide_NegativeX;
        f32 smallestDist = distSquaredPortalNegativeXCenterToPlayerView;
        if(distSquaredPortalPositiveXCenterToPlayerView < smallestDist) {
          portalOfFocus = CubeSide_PositiveX;
          smallestDist = distSquaredPortalPositiveXCenterToPlayerView;
        }
        if(distSquaredPortalNegativeYCenterToPlayerView < smallestDist) {
          portalOfFocus = CubeSide_NegativeY;
          smallestDist = distSquaredPortalNegativeYCenterToPlayerView;
        }
        if(distSquaredPortalPositiveYCenterToPlayerView < smallestDist) {
          portalOfFocus = CubeSide_PositiveY;
          smallestDist = distSquaredPortalPositiveYCenterToPlayerView;
        }
      } else { // transitioning to outside
        portalInFocus = false;
      }
    }

    b32 portalNegativeXVisible = ((glm::dot(portalNegativeXCenterToPlayerView, cubeFaceNegativeXNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_NegativeX);
    b32 portalPositiveXVisible = ((glm::dot(portalPositiveXCenterToPlayerView, cubeFacePositiveXNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_PositiveX);
    b32 portalNegativeYVisible = ((glm::dot(portalNegativeYCenterToPlayerView, cubeFaceNegativeYNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_NegativeY);
    b32 portalPositiveYVisible = ((glm::dot(portalPositiveYCenterToPlayerView, cubeFacePositiveYNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_PositiveY);

    glStencilFunc(GL_ALWAYS, // stencil function always passes
                  0x00, // reference
                  0x00); // mask

    glUseProgram(skyboxShader.id);
    setUniform(skyboxShader.id, "skybox", mainSkyboxTextureIndex);
    drawTriangles(&invertedCubePosVertexAtt);

    if(camera.thirdPerson) { // draw player if third person
      Vec3 playerCenter = calcBoundingBoxCenterPosition(player.boundingBox);
      Vec3 playerViewCenter = calcPlayerViewingPosition(&player);
      Vec3 playerBoundingBoxColor_Red = Vec3(1.0f, 0.0f, 0.0f);
      Vec3 playerViewBoxColor_White = Vec3(1.0f, 1.0f, 1.0f);
      Vec3 playerMinCoordBoxColor_Green = Vec3(0.0f, 1.0f, 0.0f);
      Vec3 playerMinCoordBoxColor_Black = Vec3(0.0f, 0.0f, 0.0f);

      Mat4 thirdPersonPlayerBoxesModelMatrix;\

      glUseProgram(shapeShader.id);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);

      // debug player bounding box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = glm::translate(mat4_identity, playerCenter) * playerBoundingBoxScaleMatrix;
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerBoundingBoxColor_Red);
      drawTriangles(&cubePosVertexAtt);

      // debug player center
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = glm::translate(mat4_identity, playerCenter) * Mat4(Mat3(0.05f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerMinCoordBoxColor_Black);
      drawTriangles(&cubePosVertexAtt);

      // debug player min coordinate box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = glm::translate(mat4_identity, player.boundingBox.min) * Mat4(Mat3(0.1f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerMinCoordBoxColor_Green);
      drawTriangles(&cubePosVertexAtt);

      // debug player view
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = glm::translate(mat4_identity, playerViewCenter) * Mat4(Mat3(0.1f));
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(Mat4), glm::value_ptr(thirdPersonPlayerBoxesModelMatrix));
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerViewBoxColor_White);
      drawTriangles(&cubePosVertexAtt);

      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if(gateIsVisible)
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
        setUniform(gateShader.id, "albedoTex", gateAlbedoTextureIndex);
        setUniform(gateShader.id, "normalTex", gateNormalTextureIndex);
        drawModel(gateModel);
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

        if(portalInFocus) {
          switch(portalOfFocus) {
            case CubeSide_NegativeX:
              glStencilMask(portalNegativeXStencilMask);
              break;
            case CubeSide_PositiveX:
              glStencilMask(portalPositiveXStencilMask);
              break;
            case CubeSide_NegativeY:
              glStencilMask(portalNegativeYStencilMask);
              break;
            case CubeSide_PositiveY:
              glStencilMask(portalPositiveYStencilMask);
              break;
          }
          drawTriangles(&invertedCubePosVertexAtt);
        } else { // no portal in focus
          if (portalNegativeXVisible)
          {
            glStencilMask(portalNegativeXStencilMask);
            drawTriangles(&cubePosVertexAtt, 6, cubeFaceNegativeXIndicesOffset);
          }

          if (portalPositiveXVisible)
          {
            glStencilMask(portalPositiveXStencilMask);
            drawTriangles(&cubePosVertexAtt, 6, cubeFacePositiveXIndicesOffset);
          }

          if (portalNegativeYVisible)
          {
            glStencilMask(portalNegativeYStencilMask);
            drawTriangles(&cubePosVertexAtt, 6, cubeFaceNegativeYIndicesOffset);
          }

          if (portalPositiveYVisible)
          {
            glStencilMask(portalPositiveYStencilMask);
            drawTriangles(&cubePosVertexAtt, 6, cubeFacePositiveYIndicesOffset);
          }
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
          drawTriangles(&invertedCubePosVertexAtt);

          // draw shape
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "baseColor", Vec3(0.4, 0.4, 1.0));
          drawModel(cubeModel);
          // draw wireframe
          setUniform(shapeShader.id, "baseColor", shapesWireFrameColor);
          drawModelWireframe(cubeModel);
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
          drawTriangles(&invertedCubePosVertexAtt);

          // draw shape
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "baseColor", Vec3(0.4, 1.0, 0.4));
          drawModel(octahedronModel);
          // draw wireframe
          setUniform(shapeShader.id, "baseColor", shapesWireFrameColor);
          drawModelWireframe(octahedronModel);
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
          drawTriangles(&invertedCubePosVertexAtt);

          // draw shape
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "baseColor", Vec3(1.0, 0.4, 0.4));
          drawModel(tetrahedronModel);
          // draw wireframe
          setUniform(shapeShader.id, "baseColor", shapesWireFrameColor);
          drawModelWireframe(tetrahedronModel);
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
          drawTriangles(&invertedCubePosVertexAtt);

          // draw shape
          glUseProgram(shapeShader.id);
          setUniform(shapeShader.id, "baseColor", Vec3(0.9, 0.9, 0.9));
          drawModel(icosahedronModel);
          // draw wireframe
          setUniform(shapeShader.id, "baseColor", shapesWireFrameColor);
          drawModelWireframe(icosahedronModel);
        }
      }
    }

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(gateShader);
  deleteShaderProgram(shapeShader);
  deleteShaderProgram(skyboxShader);
  deleteModels(modelPtrs, ArrayCount(modelPtrs));
}