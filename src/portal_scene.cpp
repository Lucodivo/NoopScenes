const vec3 defaultPlayerDimensionInMeters{0.5f, 0.25f, 1.75f}; // NOTE: ~1'7"w, 9"d, 6'h

const f32 portalScale = 3.0f;
const f32 shapeScale = 1.0f;
const vec3 portalPosition{0.0f, 0.0f, portalScale * 0.5f};
global_variable GLuint projViewModelUBOid = 0;
global_variable ShaderProgram stencilShader{};
global_variable VertexAtt portalVertexAtt{};
global_variable VertexAtt portalBackingBoxVertexAtt{};


struct Player {
  BoundingBox boundingBox;
};

enum SceneState {
  SceneState_Gate,
  SceneState_1,
  SceneState_2,
  SceneState_3,
  SceneState_4
};

enum EntityType {
  EntityType_Rotating = 1 << 0,
  EntityType_Wireframe = 1 << 1,
  EntityType_Skybox = 1 << 2,
};

struct Entity {
  ShaderProgram shaderProgram;
  Model model;
  b32 flags;
  vec3 position;
  f32 scale;
};

struct Portal {
  mat4 portalModelMat;
  mat4 backingBoxModelMat;
  vec3 normal;
  vec3 position;
  vec2 dimens;
  b32 inFocus;
  u32 stencilMask;
  u32 sceneDestination;
};

struct Scene {
  u32 entities[16];
  u32 entityCount;
  Portal portals[4]; // TODO: actually use these
  u32 portalCount;
};

struct World
{
  Camera camera;
  Player player;
  u32 currentSceneIndex;
  StopWatch stopWatch;
  Scene scenes[16];
  u32 sceneCount;
  Entity entities[128];
  u32 entityCount;
};


void drawScene(World* world, const u32 sceneIndex, u32 stencilMask = 0x00);
void drawPortals(World* world, const u32 sceneIndex);

mat4 portalBackingBoxModelMatrix(const vec3& centerPos, const vec3& desiredNormal, const f32 width, const f32 height) {
  mat4 scaleMat = scale_mat4(vec3{width, 1.0f, height});;
  mat4 rotationMat = rotate_mat4(orient(quadVertexAttNormal, desiredNormal));
  mat4 translationMat = translate_mat4(centerPos);
  return translationMat * rotationMat * scaleMat;
}

#define PORTAL_BACKING_BOX_DEPTH 0.5f

void addPortal(World* world, u32 homeSceneIndex,
                 const vec3& position, const vec3& normal, const vec2& dimens,
                 const u32 stencilMask, const u32 destinationSceneIndex) {
  Scene* homeScene = world->scenes + homeSceneIndex;
  Assert(homeScene->portalCount < ArrayCount(homeScene->portals));

  Portal portal{};
  portal.stencilMask = stencilMask;
  portal.dimens = dimens;
  portal.position = position;
  portal.normal = normal;
  portal.portalModelMat = quadModelMatrix(position, normal, dimens.x, dimens.y);
  portal.sceneDestination = destinationSceneIndex;
  portal.inFocus = false;

  mat4 translation1Mat = translate_mat4(-cubeFaceNegativeYCenter);
  mat4 scaleMat = scale_mat4(vec3{dimens.x, PORTAL_BACKING_BOX_DEPTH, dimens.y});;
  mat4 rotationMat = rotate_mat4(orient(quadVertexAttNormal, normal));
  mat4 translation2Mat = translate_mat4(position);
  portal.backingBoxModelMat = translation2Mat * rotationMat * scaleMat * translation1Mat;

  homeScene->portals[homeScene->portalCount++] = portal;
}

u32 addNewScene(World* world) {
  Assert(ArrayCount(world->scenes) > world->sceneCount);
  u32 sceneIndex = world->sceneCount++;
  world->scenes[sceneIndex] = {};
  return sceneIndex;
}

u32 addNewEntity(World* world, u32 sceneIndex) {
  Scene* scene = world->scenes + sceneIndex;
  Assert(ArrayCount(scene->entities) > scene->entityCount);
  u32 worldEntityIndex = world->entityCount++;

  world->entities[worldEntityIndex] = {};
  scene->entities[scene->entityCount++] = worldEntityIndex;
  return worldEntityIndex;
}

struct PortalFragUBO {
  vec3 directionalLightColor;
  u8 __padding1;
  vec3 ambientLightColor;
  u8 __padding2;
  vec3 directionalLightDirToSource;
  u8 __padding3;
} portalFragUbo;

inline vec3 calcBoundingBoxCenterPosition(BoundingBox box) {
  return box.min + (box.diagonal * 0.5f);
}

// assume "eyes" (FPS camera) is positioned on middle X, max Y, max Z
inline vec3 calcPlayerViewingPosition(const Player* player) {
  return player->boundingBox.min + hadamard(player->boundingBox.diagonal, {0.5f, 1.0f, 1.0f});
}

void drawTrianglesWireframe(const VertexAtt* vertexAtt) {
  glDisable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_CULL_FACE);
  drawTriangles(vertexAtt);
  glEnable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);
}

void drawPortal(const World* world, Portal* portal, Out b32* enteredPortal) {
  glUseProgram(stencilShader.id);

  // NOTE: Stencil function Example
  // GL_LEQUAL
  // Passes if ( ref & mask ) <= ( stencil & mask )
  glStencilFunc(GL_EQUAL, // func
                0xFF, // ref
                0x00); // mask // Only draw portals where the stencil is cleared
  glStencilOp(GL_KEEP, // action when stencil fails
              GL_KEEP, // action when stencil passes but depth fails
              GL_REPLACE); // action when both stencil and depth pass

  // TODO: avoid calculating this multiple times a frame
  vec3 playerViewPosition = calcPlayerViewingPosition(&world->player);

  b32 portalWasInFocus = portal->inFocus;
  vec3 portalCenterToPlayerView = playerViewPosition - portal->position;
  b32 portalViewableFromPosition = similarDirection(portalCenterToPlayerView, portal->normal);
  vec3 viewPositionPerpendicularToPortal = perpendicularTo(portalCenterToPlayerView, portal->normal);
  f32 widthDistFromCenter = magnitude(viewPositionPerpendicularToPortal.xy);
  f32 heightDistFromCenter = viewPositionPerpendicularToPortal.z;
  b32 viewerInsideDimens = widthDistFromCenter < (portal->dimens.x * 0.5f) &&
                           heightDistFromCenter < (portal->dimens.y * 0.5f);

  portal->inFocus = portalViewableFromPosition && viewerInsideDimens;
  b32 insidePortal = portalWasInFocus && !portalViewableFromPosition; // portal was in focus and now we're on the other side

  if(portal->inFocus || insidePortal) {
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portal->backingBoxModelMat);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glUseProgram(stencilShader.id);
    glStencilMask(portal->stencilMask);
    drawTriangles(&portalBackingBoxVertexAtt);
  } else if(portalViewableFromPosition) {
    glStencilMask(portal->stencilMask);
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portal->portalModelMat);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    drawTriangles(&portalVertexAtt);
  }

  *enteredPortal = insidePortal;
}

void drawPortals(World* world, const u32 sceneIndex){

  Scene* scene = world->scenes + sceneIndex;

  b32 enteredPortal = false;

  for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
    drawPortal(world, scene->portals + portalIndex, &enteredPortal);
    if(enteredPortal){
      world->currentSceneIndex = scene->portals[portalIndex].sceneDestination;
    }
  }

  // turn off writes to the stencil
  glStencilMask(0x00);

  // Draw portal scenes
  // We need to clear disable depth values so distant objects through the "portals" still get drawn
  // The portals themselves will still obey the depth of the scene, as the stencils have been rendered with depth in mind
  glClear(GL_DEPTH_BUFFER_BIT);
  for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
    drawScene(world, scene->portals[portalIndex].sceneDestination, scene->portals[portalIndex].stencilMask);
    if(enteredPortal) { scene->portals[portalIndex].inFocus = false; }
  }
}

void drawScene(World* world, const u32 sceneIndex, u32 stencilMask) {
  glStencilFunc(
          GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
          stencilMask, // ref
          0xFF); // enable which bits in reference and stored value are compared

  Scene* scene = world->scenes + sceneIndex;

  for(u32 sceneEntityIndex = 0; sceneEntityIndex < scene->entityCount; ++sceneEntityIndex) {
    u32 entityIndex = scene->entities[sceneEntityIndex];
    Entity* entity = world->entities + entityIndex;
    ShaderProgram shader = entity->shaderProgram;

    mat4 modelMatrix = translate_mat4(entity->position) * scale_mat4(entity->scale);
    if(entity->flags & EntityType_Rotating) {
      modelMatrix = rotate_mat4(30.0f * RadiansPerDegree * world->stopWatch.totalElapsed, {0.0f, 0.0f, 1.0f}) * modelMatrix;
    }

    // all shapes use the same model matrix
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &modelMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glUseProgram(shader.id);
    for(u32 meshIndex = 0; meshIndex < entity->model.meshCount; ++meshIndex) {
      Mesh* mesh = entity->model.meshes + meshIndex;
      if(entity->flags & EntityType_Skybox) {
        bindActiveTextureCubeMap(0, mesh->textureData.albedoTextureId);
        setUniform(shader.id, "albedoTex", 0);
      } else {
        if(mesh->textureData.baseColor.w != 0.0f) {
          setUniform(shader.id, "baseColor", mesh->textureData.baseColor.xyz);
        }
        if(mesh->textureData.albedoTextureId != TEXTURE_ID_NO_TEXTURE) {
          bindActiveTextureSampler2d(0, mesh->textureData.albedoTextureId);
          setUniform(shader.id, "albedoTex", 0);
        }
        if(mesh->textureData.normalTextureId != TEXTURE_ID_NO_TEXTURE) {
          bindActiveTextureSampler2d(1, mesh->textureData.normalTextureId);
          setUniform(shader.id, "normalTex", 1);
        }
      }

      drawTriangles(&mesh->vertexAtt);

      if(entity->flags & EntityType_Wireframe) {
        glUseProgram(stencilShader.id);
        drawTrianglesWireframe(&mesh->vertexAtt);
      }
    }
  }
}

void drawSceneWithPortals(World* world)
{
  // draw scene
  drawScene(world, world->currentSceneIndex);
  // draw portals
  drawPortals(world, world->currentSceneIndex);
}

void portalScene(GLFWwindow* window) {
  vec2_u32 windowExtent = getWindowExtent();
  const vec2_u32 initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  World world{};

  VertexAtt cubePosVertexAtt = cubePosVertexAttBuffers();
  portalVertexAtt = quadPosVertexAttBuffers();
  portalBackingBoxVertexAtt = cubePosVertexAttBuffers(true, true);

  ShaderProgram albedoNormalTexShader = createShaderProgram(gateVertexShaderFileLoc, gateFragmentShaderFileLoc);
  ShaderProgram singleColorShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);
  stencilShader = createShaderProgram(posVertexShaderFileLoc, blackFragmentShaderFileLoc);

  ProjectionViewModelUBO projectionViewModelUbo;

  const f32 gateScale = portalScale;
  const vec3 gatePosition = portalPosition;

  const vec3 shapePosition = portalPosition;

  world.player.boundingBox.diagonal = defaultPlayerDimensionInMeters;
  world.player.boundingBox.min = {-(world.player.boundingBox.diagonal.x * 0.5f), -10.0f - (world.player.boundingBox.diagonal.y * 0.5f), 0.0f};

  vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&world.player);
  vec3 firstPersonCameraInitFocus{gatePosition.x, gatePosition.y, firstPersonCameraInitPosition.z};
  lookAt_FirstPerson(firstPersonCameraInitPosition, firstPersonCameraInitFocus, &world.camera);

  mat4 playerBoundingBoxScaleMatrix = scale_mat4(world.player.boundingBox.diagonal);
  const f32 originalProjectionDepthNear = 0.1f;
  const f32 originalProjectionDepthFar = 200.0f;
  const f32 fovY = fieldOfView(13.5f, 25.0f);
  projectionViewModelUbo.projection = perspective(fovY, aspectRatio, originalProjectionDepthNear, originalProjectionDepthFar);

  u32 gateSceneIndex = addNewScene(&world);
  u32 tetrahedronSceneIndex = addNewScene(&world);
  u32 octahedronSceneIndex = addNewScene(&world);
  u32 paperSceneIndex = addNewScene(&world);
  u32 icosahedronSceneIndex = addNewScene(&world);

  world.currentSceneIndex = gateSceneIndex;
  u32 gateEntityIndex;
  {
    gateEntityIndex = addNewEntity(&world, gateSceneIndex);
    Entity* gate = world.entities + gateEntityIndex;
    loadModel(gateModelLoc, &gate->model);
    gate->model.boundingBox.min *= gateScale;
    gate->model.boundingBox.min += gatePosition;
    gate->model.boundingBox.diagonal *= gateScale;
    gate->position = gatePosition;
    gate->scale = gateScale;
    gate->shaderProgram = albedoNormalTexShader;

    u32 caveSkyboxIndex = addNewEntity(&world, gateSceneIndex);
    Entity* caveSkybox = world.entities + caveSkyboxIndex;
    skyBoxModel(caveFaceLocations, &caveSkybox->model);
    caveSkybox->position = {0.0f, 0.0f, 0.0f};
    caveSkybox->scale = 1.0f;
    caveSkybox->shaderProgram = skyboxShader;
    caveSkybox->flags = EntityType_Skybox;
  }

  {
    u32 tetrahedronEntityIndex = addNewEntity(&world, tetrahedronSceneIndex);
    Entity* tetrahedron = world.entities + tetrahedronEntityIndex;
    loadModel(tetrahedronModelLoc, &tetrahedron->model);
    tetrahedron->model.meshes[0].textureData.baseColor = vec4{1.0f, 0.4f, 0.4f, 1.0f};
    tetrahedron->position = shapePosition;
    tetrahedron->scale = shapeScale;
    tetrahedron->shaderProgram = singleColorShader;
    tetrahedron->flags = EntityType_Rotating | EntityType_Wireframe;

    u32 yellowCloudSkyboxEntityIndex = addNewEntity(&world, tetrahedronSceneIndex);
    Entity* yellowCloudSkybox = world.entities + yellowCloudSkyboxEntityIndex;
    skyBoxModel(yellowCloudFaceLocations, &yellowCloudSkybox->model);
    yellowCloudSkybox->position = {0.0f, 0.0f, 0.0f};
    yellowCloudSkybox->scale = 1.0f;
    yellowCloudSkybox->shaderProgram = skyboxShader;
    yellowCloudSkybox->flags = EntityType_Skybox;
  }

  {
    u32 octahedronEntityIndex = addNewEntity(&world, octahedronSceneIndex);
    Entity* octahedron = world.entities + octahedronEntityIndex;
    loadModel(octahedronModelLoc, &octahedron->model);
    octahedron->model.meshes[0].textureData.baseColor = vec4{0.4f, 1.0f, 0.4f, 1.0f};
    octahedron->position = shapePosition;
    octahedron->scale = shapeScale;
    octahedron->shaderProgram = singleColorShader;
    octahedron->flags = EntityType_Rotating | EntityType_Wireframe;

    u32 interstellarSkyboxEntityIndex = addNewEntity(&world, octahedronSceneIndex);
    Entity* interstellarSkybox = world.entities + interstellarSkyboxEntityIndex;
    skyBoxModel(skyboxInterstellarFaceLocations, &interstellarSkybox->model);
    interstellarSkybox->position = {0.0f, 0.0f, 0.0f};
    interstellarSkybox->scale = 1.0f;
    interstellarSkybox->shaderProgram = skyboxShader;
    interstellarSkybox->flags = EntityType_Skybox;
  }

  {
    u32 paperEntityIndex = addNewEntity(&world, paperSceneIndex);
    Entity* paper = world.entities + paperEntityIndex;
    loadModel(paperModelLoc, &paper->model);
    paper->model.meshes[0].textureData.baseColor = vec4{0.4f, 0.4f, 1.0f, 1.0f};
    paper->position = shapePosition;
    paper->scale = shapeScale;
    paper->shaderProgram = singleColorShader;
    paper->flags = EntityType_Rotating | EntityType_Wireframe;

    u32 calmSeaSkyboxEntityIndex = addNewEntity(&world, paperSceneIndex);
    Entity* calmSeaSkybox = world.entities + calmSeaSkyboxEntityIndex;
    skyBoxModel(calmSeaFaceLocations, &calmSeaSkybox->model);
    calmSeaSkybox->position = {0.0f, 0.0f, 0.0f};
    calmSeaSkybox->scale = 1.0f;
    calmSeaSkybox->shaderProgram = skyboxShader;
    calmSeaSkybox->flags = EntityType_Skybox;
  }

  {
    u32 icosahedronEntityIndex = addNewEntity(&world, icosahedronSceneIndex);
    Entity* icosahedron = world.entities + icosahedronEntityIndex;
    loadModel(icosahedronModelLoc, &icosahedron->model);
    icosahedron->model.meshes[0].textureData.baseColor = vec4{0.9f, 0.9f, 0.9f, 1.0f};
    icosahedron->position = shapePosition;
    icosahedron->scale = shapeScale;
    icosahedron->shaderProgram = singleColorShader;
    icosahedron->flags = EntityType_Rotating | EntityType_Wireframe;

    u32 pollutedEarthSkyboxEntityIndex = addNewEntity(&world, icosahedronSceneIndex);
    Entity* pollutedEarthSkybox = world.entities + pollutedEarthSkyboxEntityIndex;
    skyBoxModel(pollutedEarthFaceLocations, &pollutedEarthSkybox->model);
    pollutedEarthSkybox->position = {0.0f, 0.0f, 0.0f};
    pollutedEarthSkybox->scale = 1.0f;
    pollutedEarthSkybox->shaderProgram = skyboxShader;
    pollutedEarthSkybox->flags = EntityType_Skybox;
  }

  { // portals
    addPortal(&world, gateEntityIndex,
              (cubeFaceNegativeXCenter * portalScale) + portalPosition,
              negativeXNormal,
              vec2{portalScale, portalScale},
              0x01,
              paperSceneIndex
    );

    addPortal(&world, paperSceneIndex,
              (cubeFaceNegativeXCenter * portalScale) + portalPosition,
              positiveXNormal,
              vec2{portalScale, portalScale},
              0x01,
              gateEntityIndex
    );

    addPortal(&world, gateEntityIndex,
              (cubeFacePositiveXCenter * portalScale) + portalPosition,
              positiveXNormal,
              vec2{portalScale, portalScale},
              0x02,
              octahedronSceneIndex
    );

    addPortal(&world, octahedronSceneIndex,
              (cubeFacePositiveXCenter * portalScale) + portalPosition,
              negativeXNormal,
              vec2{portalScale, portalScale},
              0x02,
              gateEntityIndex
    );

    addPortal(&world, gateEntityIndex,
              (cubeFaceNegativeYCenter * portalScale) + portalPosition,
              negativeYNormal,
              vec2{portalScale, portalScale},
              0x03,
              tetrahedronSceneIndex
    );

    addPortal(&world, tetrahedronSceneIndex,
              (cubeFaceNegativeYCenter * portalScale) + portalPosition,
              positiveYNormal,
              vec2{portalScale, portalScale},
              0x03,
              gateEntityIndex
    );

    addPortal(&world, gateEntityIndex,
              (cubeFacePositiveYCenter * portalScale) + portalPosition,
              positiveYNormal,
              vec2{portalScale, portalScale},
              0x04,
              icosahedronSceneIndex
    );

    addPortal(&world, icosahedronSceneIndex,
              (cubeFacePositiveYCenter * portalScale) + portalPosition,
              negativeYNormal,
              vec2{portalScale, portalScale},
              0x04,
              gateEntityIndex
    );
  }

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

  portalFragUbo.directionalLightColor = {0.5f, 0.5f, 0.5f};
  portalFragUbo.ambientLightColor = {0.2f, 0.2f, 0.2f};
  portalFragUbo.directionalLightDirToSource = {1.0f, 1.0f, 1.0f};

  // UBOs
  GLuint portalFragUBOid;
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

  world.stopWatch = createStopWatch();
  while(glfwWindowShouldClose(window) == GL_FALSE)
  {
    loadInputStateForFrame(window);
    updateStopWatch(&world.stopWatch);

    vec3 playerCenter = calcBoundingBoxCenterPosition(world.player.boundingBox);
    vec3 playerViewPosition = calcPlayerViewingPosition(&world.player);

    if (isActive(KeyboardInput_Esc))
    {
      glfwSetWindowShouldClose(window, true);
      break;
    }

    // toggle fullscreen/window mode if alt + enter
    if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
      windowExtent = toggleWindowSize(window, initWindowExtent.width, initWindowExtent.height);
      aspectRatio = f32(windowExtent.width) / windowExtent.height;
      glViewport(0, 0, windowExtent.width, windowExtent.height);

      adjustAspectPerspProj(&projectionViewModelUbo.projection, fovY, aspectRatio);
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, projection), sizeof(mat4), &projectionViewModelUbo.projection);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // gather input
    b32 leftShiftIsActive = isActive(KeyboardInput_Shift_Left);
    b32 leftIsActive = isActive(KeyboardInput_A) || isActive(KeyboardInput_Left);
    b32 rightIsActive = isActive(KeyboardInput_D) || isActive(KeyboardInput_Right);
    b32 upIsActive = isActive(KeyboardInput_W) || isActive(KeyboardInput_Up);
    b32 downIsActive = isActive(KeyboardInput_S) || isActive(KeyboardInput_Down);
    b32 tabHotPress = hotPress(KeyboardInput_Tab);
    vec2_f64 mouseDelta = getMouseDelta();

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
        playerDelta = playerMovementDirection * playerMovementSpeed * world.stopWatch.delta;
      }

      // TODO: Do not apply immediately, check for collisions
      world.player.boundingBox.min += playerDelta;

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
    glStencilFunc(GL_ALWAYS, // stencil function always passes
                  0x00, // reference
                  0x00); // mask
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // universal matrices in UBO
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
    // TODO: don't need to set projection matrix right now but will in the future so keeping
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &projectionViewModelUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if(world.camera.thirdPerson) { // draw player if third person
      vec3 playerCenter = calcBoundingBoxCenterPosition(world.player.boundingBox);
      vec3 playerViewCenter = calcPlayerViewingPosition(&world.player);
      vec3 playerBoundingBoxColor_Red{1.0f, 0.0f, 0.0f};
      vec3 playerViewBoxColor_White{1.0f, 1.0f, 1.0f};
      vec3 playerMinCoordBoxColor_Green{0.0f, 1.0f, 0.0f};
      vec3 playerMinCoordBoxColor_Black{0.0f, 0.0f, 0.0f};

      mat4 thirdPersonPlayerBoxesModelMatrix;

      glUseProgram(singleColorShader.id);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);

      // debug player bounding box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerCenter) * playerBoundingBoxScaleMatrix;
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerBoundingBoxColor_Red);
      drawTriangles(&cubePosVertexAtt);

      // debug player center
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerCenter) * scale_mat4(0.05f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerMinCoordBoxColor_Black);
      drawTriangles(&cubePosVertexAtt);

      // debug player min coordinate box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(world.player.boundingBox.min) * scale_mat4(0.1f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerMinCoordBoxColor_Green);
      drawTriangles(&cubePosVertexAtt);

      // debug player view
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerViewCenter) * scale_mat4(0.1f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerViewBoxColor_White);
      drawTriangles(&cubePosVertexAtt);

      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    drawSceneWithPortals(&world);

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(albedoNormalTexShader);
  deleteShaderProgram(singleColorShader);
  deleteShaderProgram(skyboxShader);
}