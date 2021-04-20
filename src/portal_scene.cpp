#include "vertex_attributes.h"
#include "shader_program.h"
#include "file_locations.h"
#include "glfw_util.cpp"
#include "util.h"
#include "textures.h"
#include "camera.h"
#include "uniform_buffer_object_structs.h"
#include "model.h"
#include "noop_math.h"

// TODO: remove
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define NO_STENCIL_MASK 0

const vec3 defaultPlayerDimensionInMeters{0.5f, 0.25f, 1.75f}; // NOTE: ~1'7"w, 9"d, 6'h

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

inline vec3 calcBoundingBoxCenterPosition(BoundingBox box) {
  return box.min + (box.dimensionInMeters * 0.5f);
}

// assume "eyes" (FPS camera) is positioned on middle X, max Y, max Z
inline vec3 calcPlayerViewingPosition(Player* player) {
  return player->boundingBox.min + hadamard(player->boundingBox.dimensionInMeters, {0.5f, 1.0f, 1.0f});
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
  vec3 position;
};

struct World
{
  Camera camera;
  Player player;
  SceneState scene;
  std::vector<Entity> entities;
};

bool insideBox(BoundingBox boundingBox, vec3 position) {
  const vec3 boundingBoxMax = boundingBox.min + boundingBox.dimensionInMeters;
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
  const vec3 portalPosition{0.0f, 0.0f, portalScale * 0.5f};
  BoundingBox portalBoundingBox = {(cubeVertAttBoundingBox.min * portalScale) + portalPosition, cubeVertAttBoundingBox.dimensionInMeters * portalScale };

  const f32 gateScale = portalScale;
  const vec3 gatePosition = portalPosition;

  const f32 shapeScale = 1.0f;
  const vec3 shapePosition = portalPosition;
  vec3 shapesWireFrameColor{0.1f, 0.1f, 0.1f};

  Model gateModel, tetrahedronModel, octahedronModel, cubeModel, icosahedronModel;
  Model* modelPtrs[] = { &gateModel, &tetrahedronModel, &octahedronModel, &cubeModel, &icosahedronModel };
  const char* modelLocs[] = { gateModelLoc, tetrahedronModelLoc, octahedronModelLoc, cubeModelLoc, icosahedronModelLoc };
  loadModels(modelLocs, ArrayCount(modelPtrs), modelPtrs);

  gateModel.boundingBox.min *= gateScale;
  gateModel.boundingBox.min += gatePosition;
  gateModel.boundingBox.dimensionInMeters *= gateScale;

  Player player;
  player.boundingBox.dimensionInMeters = defaultPlayerDimensionInMeters;
  player.boundingBox.min = {-(player.boundingBox.dimensionInMeters.x * 0.5f), -10.0f - (player.boundingBox.dimensionInMeters.y * 0.5f), 0.0f};

  vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&player);
  vec3 firstPersonCameraInitFocus{gatePosition.x, gatePosition.y, firstPersonCameraInitPosition.z};
  lookAt_FirstPerson(firstPersonCameraInitPosition, firstPersonCameraInitFocus, &world.camera);

  VertexAtt cubePosVertexAtt = initializeCubePositionVertexAttBuffers();
  VertexAtt invertedCubePosVertexAtt = initializeCubePositionVertexAttBuffers(true);

  mat4 shapeModelMatrix = translate_mat4(shapePosition) * scale_mat4(shapeScale);
  mat4 portalModelMatrix = translate_mat4(portalPosition) * scale_mat4(portalScale);
  mat4 gateModelMatrix = translate_mat4(gatePosition) * scale_mat4(gateScale);
  mat4 playerBoundingBoxScaleMatrix = scale_mat4(player.boundingBox.dimensionInMeters);
  const f32 originalProjectionDepthNear = 0.1f;
  const f32 originalProjectionDepthFar = 200.0f;
  // TODO: BIG TIME
  glm::mat4 p = glm::perspective(45.0f * RadiansPerDegree, aspectRatio, originalProjectionDepthNear, originalProjectionDepthFar);
  projectionViewModelUbo.projection = {
          p[0][0], p[0][1], p[0][2], p[0][3],
          p[1][0], p[1][1], p[1][2], p[1][3],
          p[2][0], p[2][1], p[2][2], p[2][3],
          p[3][0], p[3][1], p[3][2], p[3][3],
  };

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
    vec3 directionalLightColor;
    u8 __padding1;
    vec3 ambientLightColor;
    u8 __padding2;
    vec3 directionalLightDirToSource;
    u8 __padding3;
  } portalFragUbo;

  portalFragUbo.directionalLightColor = {0.5f, 0.5f, 0.5f};
  portalFragUbo.ambientLightColor = {0.2f, 0.2f, 0.2f};
  portalFragUbo.directionalLightDirToSource = {1.0f, 1.0f, 1.0f};

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

  vec3 portalNegativeXCenter = cubeFaceNegativeXCenter * portalScale;
  vec3 portalPositiveXCenter = cubeFacePositiveXCenter * portalScale;
  vec3 portalNegativeYCenter = cubeFaceNegativeYCenter * portalScale;
  vec3 portalPositiveYCenter = cubeFacePositiveYCenter * portalScale;

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

    vec3 playerCenter = calcBoundingBoxCenterPosition(player.boundingBox);
    vec3 playerViewPosition = calcPlayerViewingPosition(&player);

    if (isActive(KeyboardInput_Esc))
    {
      glfwSetWindowShouldClose(window, true);
      break;
    }

    // toggle fullscreen/window mode if alt + enter
    if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
      windowExtent = toggleWindowSize(window, initWindowExtent.width, initWindowExtent.height);
      aspectRatio = f32(windowExtent.width) / windowExtent.height;
      // TODO: update projection matrix for potentially new aspect ratio
      glViewport(0, 0, windowExtent.width, windowExtent.height);
    }

    // gather input
    b32 leftShiftIsActive = isActive(KeyboardInput_Shift_Left);
    b32 leftIsActive = isActive(KeyboardInput_A) || isActive(KeyboardInput_Left);
    b32 rightIsActive = isActive(KeyboardInput_D) || isActive(KeyboardInput_Right);
    b32 upIsActive = isActive(KeyboardInput_W) || isActive(KeyboardInput_Up);
    b32 downIsActive = isActive(KeyboardInput_S) || isActive(KeyboardInput_Down);
    b32 tabHotPress = hotPress(KeyboardInput_Tab);
    vec2_64 mouseDelta = getMouseDelta();

    // gather input for movement and camera changes
    {
      b32 lateralMovement = leftIsActive != rightIsActive;
      b32 forwardMovement = upIsActive != downIsActive;
      vec3 playerDelta{};
      if (lateralMovement || forwardMovement)
      {
        f32 playerMovementSpeed = leftShiftIsActive ? 5.0f : 1.0f;

        // Camera movement direction
        vec3 playerMovementDirection{};
        if (lateralMovement)
        {
          playerMovementDirection += rightIsActive ? world.camera.right : -world.camera.right;
        }

        if (forwardMovement)
        {
          playerMovementDirection += upIsActive ? world.camera.forward : -world.camera.forward;
        }

        playerMovementDirection = normalize(playerMovementDirection.x, playerMovementDirection.y, 0.0);
        playerDelta = playerMovementDirection * playerMovementSpeed * stopWatch.delta;
      }
      player.boundingBox.min += playerDelta;

      if(tabHotPress) { // switch between third and first person
        vec3 xyForward = normalize(world.camera.forward.x, world.camera.forward.y, 0.0f);

        if(!world.camera.thirdPerson) {
          lookAt_ThirdPerson(playerCenter, xyForward, &world.camera);
        } else { // camera is first person now
          vec3 focus = playerViewPosition + xyForward;
          lookAt_FirstPerson(playerViewPosition, focus, &world.camera);
        }
      }

      const f32 mouseDeltaMultConst = 0.001f;
      if(world.camera.thirdPerson) {
        updateCamera_ThirdPerson(&world.camera, playerCenter, f32(-mouseDelta.y * mouseDeltaMultConst),f32(-mouseDelta.x * 0.001f));
      } else {
        updateCamera_FirstPerson(&world.camera, playerDelta, f32(-mouseDelta.y * mouseDeltaMultConst), f32(-mouseDelta.x * 0.001f));
      }

      projectionViewModelUbo.view = getViewMat(world.camera);
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
    b32 gateIsInFront = dot(world.camera.forward, normalize(gatePosition - world.camera.origin)) > 0;
    b32 insideGate = insideBox(gateModel.boundingBox, playerViewPosition);
    b32 gateIsVisible = gateIsInFront || insideGate;

    vec3 portalNegativeXCenterToPlayerView = playerViewPosition - portalNegativeXCenter;
    vec3 portalPositiveXCenterToPlayerView = playerViewPosition - portalPositiveXCenter;
    vec3 portalNegativeYCenterToPlayerView = playerViewPosition - portalNegativeYCenter;
    vec3 portalPositiveYCenterToPlayerView = playerViewPosition - portalPositiveYCenter;
    f32 distSquaredPortalNegativeXCenterToPlayerView = magnitudeSquared(portalNegativeXCenterToPlayerView);
    f32 distSquaredPortalPositiveXCenterToPlayerView = magnitudeSquared(portalPositiveXCenterToPlayerView);
    f32 distSquaredPortalNegativeYCenterToPlayerView = magnitudeSquared(portalNegativeYCenterToPlayerView);
    f32 distSquaredPortalPositiveYCenterToPlayerView = magnitudeSquared(portalPositiveYCenterToPlayerView);

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

    b32 portalNegativeXVisible = ((dot(portalNegativeXCenterToPlayerView, cubeFaceNegativeXNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_NegativeX);
    b32 portalPositiveXVisible = ((dot(portalPositiveXCenterToPlayerView, cubeFacePositiveXNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_PositiveX);
    b32 portalNegativeYVisible = ((dot(portalNegativeYCenterToPlayerView, cubeFaceNegativeYNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_NegativeY);
    b32 portalPositiveYVisible = ((dot(portalPositiveYCenterToPlayerView, cubeFacePositiveYNormal) > 0.0f) && gateIsVisible && !portalInFocus) ||
                                 (portalInFocus && portalOfFocus == CubeSide_PositiveY);

    glStencilFunc(GL_ALWAYS, // stencil function always passes
                  0x00, // reference
                  0x00); // mask

    glUseProgram(skyboxShader.id);
    setUniform(skyboxShader.id, "skybox", mainSkyboxTextureIndex);
    drawTriangles(&invertedCubePosVertexAtt);

    if(world.camera.thirdPerson) { // draw player if third person
      vec3 playerCenter = calcBoundingBoxCenterPosition(player.boundingBox);
      vec3 playerViewCenter = calcPlayerViewingPosition(&player);
      vec3 playerBoundingBoxColor_Red{1.0f, 0.0f, 0.0f};
      vec3 playerViewBoxColor_White{1.0f, 1.0f, 1.0f};
      vec3 playerMinCoordBoxColor_Green{0.0f, 1.0f, 0.0f};
      vec3 playerMinCoordBoxColor_Black{0.0f, 0.0f, 0.0f};

      mat4 thirdPersonPlayerBoxesModelMatrix;

      glUseProgram(shapeShader.id);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);

      // debug player bounding box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerCenter) * playerBoundingBoxScaleMatrix;
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerBoundingBoxColor_Red);
      drawTriangles(&cubePosVertexAtt);

      // debug player center
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerCenter) * scale_mat4(0.05f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerMinCoordBoxColor_Black);
      drawTriangles(&cubePosVertexAtt);

      // debug player min coordinate box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(player.boundingBox.min) * scale_mat4(0.1f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(shapeShader.id, "baseColor", playerMinCoordBoxColor_Green);
      drawTriangles(&cubePosVertexAtt);

      // debug player view
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerViewCenter) * scale_mat4(0.1f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
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
        portalFragUbo.directionalLightDirToSource = normalize(cosTime, sinTime, cosTime); // orbit xyplane (ground), oscillate up and down
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(PortalFragUBO, directionalLightDirToSource), sizeof(vec4), &portalFragUbo.directionalLightDirToSource);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &gateModelMatrix);
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
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portalModelMatrix);
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
        shapeModelMatrix = rotate_mat4(30.0f * RadiansPerDegree * stopWatch.delta, {0.0f, 0.0f, 1.0f}) * shapeModelMatrix;

        // all shapes use the same model matrix
        glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
        glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &shapeModelMatrix);
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
          setUniform(shapeShader.id, "baseColor", vec3{0.4f, 0.4f, 1.0f});
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
          setUniform(shapeShader.id, "baseColor", vec3{0.4f, 1.0f, 0.4f});
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
          setUniform(shapeShader.id, "baseColor", vec3{1.0f, 0.4f, 0.4f});
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
          setUniform(shapeShader.id, "baseColor", vec3{0.9f, 0.9f, 0.9f});
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