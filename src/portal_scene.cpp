#define PORTAL_BACKING_BOX_DEPTH 0.5f

const vec3 defaultPlayerDimensionInMeters{0.5f, 0.25f, 1.75f}; // NOTE: ~1'7"w, 9"d, 6'h
const vec3 gateScale{3.0f, 3.0f, 3.0f};
const f32 gateYaw = 0.0f;
const vec2 portalDimens{gateScale.x, gateScale.z};
const vec3 shapeScale{1.0f, 1.0f, 1.0f};
const vec3 gatePosition{0.0f, 0.0f, gateScale.z * 0.5f};
const vec3 shapePosition = gatePosition;
const vec3 skyboxPosition{0.0f, 0.0f, 0.0f};
const vec3 skyboxScale{1.0f, 1.0f, 1.0f};
const u32 destGateStencilMask = 0x01;
const u32 destTetrahedronStencilMask = 0x02;
const u32 destOctahedronStencilMask = 0x03;
const u32 destIcosahedronStencilMask = 0x04;
const u32 destPaperStencilMask = 0x05;

const s32 skyboxActiveTextureIndex = 0;
const s32 albedoActiveTextureIndex = 1;
const s32 normalActiveTextureIndex = 2;

global_variable GLuint projViewModelGlobalUBOid = 0;
global_variable ShaderProgram stencilShader{};
global_variable VertexAtt portalVertexAtt{};
global_variable VertexAtt portalBackingBoxVertexAtt{};

struct Player {
  BoundingBox boundingBox;
};

enum EntityType {
  EntityType_Rotating = 1 << 0,
  EntityType_Wireframe = 1 << 1
};

struct Entity {
  ShaderProgram shaderProgram;
  BoundingBox boundingBox;
  u32 modelIndex;
  b32 flags;
  vec3 position;
  vec3 scale;
  f32 yaw; // NOTE: Radians. 0 rads starts at {0, -1} and goes around the xy-plane in a CCW as seen from above
};

struct Portal {
  vec3 normal;
  vec3 centerPosition;
  vec2 dimens;
  b32 inFocus;
  u32 stencilMask;
  u32 sceneDestination;
};

struct Scene {
  // TODO: should the scene keep track of its own index in the world?
  Entity entities[16];
  u32 entityCount;
  Portal portals[4];
  u32 portalCount;
  GLuint skyboxTexture;
};

struct World
{
  Camera camera;
  Player player;
  u32 currentSceneIndex;
  StopWatch stopWatch;
  Scene scenes[16];
  u32 sceneCount;
  Model models[128];
  u32 modelCount;
};


void drawScene(World* world, const u32 sceneIndex, u32 stencilMask = 0x00);
void drawPortals(World* world, const u32 sceneIndex);

void addPortal(World* world, u32 homeSceneIndex,
               const vec3& centerPosition, const vec3& normal, const vec2& dimens,
               const u32 stencilMask, const u32 destinationSceneIndex) {
  Scene* homeScene = world->scenes + homeSceneIndex;
  Assert(homeScene->portalCount < ArrayCount(homeScene->portals));

  Portal portal{};
  portal.stencilMask = stencilMask;
  portal.dimens = dimens;
  portal.centerPosition = centerPosition;
  portal.normal = normal;
  portal.sceneDestination = destinationSceneIndex;
  portal.inFocus = false;

  homeScene->portals[homeScene->portalCount++] = portal;
}

u32 addNewScene(World* world) {
  Assert(ArrayCount(world->scenes) > world->sceneCount);
  u32 sceneIndex = world->sceneCount++;
  world->scenes[sceneIndex] = {};
  return sceneIndex;
}

u32 addNewEntity(World* world, u32 sceneIndex, u32 modelIndex,
                 vec3 pos, vec3 scale, f32 yaw,
                 ShaderProgram shader, b32 entityTypeFlags = 0) {
  Scene* scene = world->scenes + sceneIndex;
  Assert(ArrayCount(scene->entities) > scene->entityCount);
  u32 sceneEntityIndex = scene->entityCount++;
  Entity* entity = scene->entities + sceneEntityIndex;
  *entity = {};
  entity->modelIndex = modelIndex;
  entity->boundingBox = world->models[modelIndex].boundingBox;
  entity->boundingBox.min = hadamard(entity->boundingBox.min, scale);
  entity->boundingBox.min += pos;
  entity->boundingBox.diagonal = hadamard(entity->boundingBox.diagonal, scale);
  entity->position = pos;
  entity->scale = scale;
  entity->shaderProgram = shader;
  entity->flags = entityTypeFlags;
  return sceneEntityIndex;
}

u32 addNewModel(World* world, const char* modelFileLoc) {
  u32 modelIndex = world->modelCount++;
  loadModel(modelFileLoc, world->models + modelIndex);
  return modelIndex;
}

u32 addNewModel_Skybox(World* world) {
  u32 modelIndex = world->modelCount++;
  Model* model = world->models + modelIndex;
  model->boundingBox = cubeVertAttBoundingBox;
  model->meshes = new Mesh[1];
  model->meshCount = 1;
  model->meshes[0].vertexAtt = cubePosVertexAttBuffers(true);
  model->meshes[0].textureData = {};
  model->meshes[0].textureData.albedoTextureId = TEXTURE_ID_NO_TEXTURE;
  model->meshes[0].textureData.normalTextureId = TEXTURE_ID_NO_TEXTURE;
  return modelIndex;
}

struct {
  vec3 directionalLightColor;
  u8 __padding1;
  vec3 ambientLightColor;
  u8 __padding2;
  vec3 directionalLightDirToSource;
  u8 __padding3;
} gateSceneFragUbo;

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

void calcBackingBoxStencilModelMatFromPortalModelMat(const mat4& portalModelMat, mat4* outModelMat) {
  *outModelMat = portalModelMat * scale_mat4(vec3{1.0f, PORTAL_BACKING_BOX_DEPTH, 1.0f}) * translate_mat4(-cubeFaceNegativeYCenter);
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
  vec3 portalCenterToPlayerView = playerViewPosition - portal->centerPosition;
  b32 portalViewableFromPosition = similarDirection(portalCenterToPlayerView, portal->normal);
  vec3 viewPositionPerpendicularToPortal = perpendicularTo(portalCenterToPlayerView, portal->normal);
  f32 widthDistFromCenter = magnitude(viewPositionPerpendicularToPortal.xy);
  f32 heightDistFromCenter = viewPositionPerpendicularToPortal.z;
  b32 viewerInsideDimens = widthDistFromCenter < (portal->dimens.x * 0.5f) &&
                           heightDistFromCenter < (portal->dimens.y * 0.5f);

  portal->inFocus = portalViewableFromPosition && viewerInsideDimens;
  b32 insidePortal = portalWasInFocus && !portalViewableFromPosition; // portal was in focus and now we're on the other side

  mat4 portalModelMat = quadModelMatrix(portal->centerPosition, portal->normal, portal->dimens.x, portal->dimens.y);
  if(portal->inFocus || insidePortal) {
    mat4 portalBackingBoxStencilModelMat;
    calcBackingBoxStencilModelMatFromPortalModelMat(portalModelMat, &portalBackingBoxStencilModelMat);

    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portalBackingBoxStencilModelMat);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glUseProgram(stencilShader.id);
    glStencilMask(portal->stencilMask);
    drawTriangles(&portalBackingBoxVertexAtt);
  } else if(portalViewableFromPosition) {
    glStencilMask(portal->stencilMask);
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portalModelMat);
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

  bindActiveTextureCubeMap(skyboxActiveTextureIndex, scene->skyboxTexture);

  for(u32 sceneEntityIndex = 0; sceneEntityIndex < scene->entityCount; ++sceneEntityIndex) {
    Entity* entity = &scene->entities[sceneEntityIndex];
    ShaderProgram shader = entity->shaderProgram;

    // TODO: Dead logic for parent entities, bring back when looked into further
//    vec3 absolutePos = entity->position;
//    vec3 absoluteScale = entity->scale;
//    Entity* traversalEntity = entity;
//    while(traversalEntity->parentEntityIndex != NULL_INDEX) {
//      u32 parentEntityIndex = traversalEntity->parentEntityIndex;
//      Assert(parentEntityIndex < scene->entityCount);
//      traversalEntity = scene->entities + parentEntityIndex;
//      absolutePos += traversalEntity->position;
//      absoluteScale = hadamard(traversalEntity->scale, absoluteScale);
//    }

    mat4 modelMatrix = translate_mat4(entity->position) * rotate_xyPlane_mat4(entity->yaw) * scale_mat4(entity->scale);

    // all shapes use the same modelIndex matrix
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &modelMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glUseProgram(shader.id);
    setSamplerCube(shader.id, "skyboxTex", skyboxActiveTextureIndex);
    Model model = world->models[entity->modelIndex];
    for(u32 meshIndex = 0; meshIndex < model.meshCount; ++meshIndex) {
      Mesh* mesh = model.meshes + meshIndex;
      if(mesh->textureData.baseColor.w != 0.0f) {
        setUniform(shader.id, "baseColor", mesh->textureData.baseColor.xyz);
      }
      if(mesh->textureData.albedoTextureId != TEXTURE_ID_NO_TEXTURE) {
        bindActiveTextureSampler2d(albedoActiveTextureIndex, mesh->textureData.albedoTextureId);
        setSampler2D(shader.id, "albedoTex", albedoActiveTextureIndex);
      }
      if(mesh->textureData.normalTextureId != TEXTURE_ID_NO_TEXTURE) {
        bindActiveTextureSampler2d(normalActiveTextureIndex, mesh->textureData.normalTextureId);
        setSampler2D(shader.id, "normalTex", normalActiveTextureIndex);
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

void updateEntities(World* world) {
  for(u32 sceneIndex = 0; sceneIndex < world->sceneCount; ++sceneIndex) {
    Scene* scene = world->scenes + sceneIndex;
    for(u32 entityIndex = 0; entityIndex < scene->entityCount; ++entityIndex) {
      Entity* entity = scene->entities + entityIndex;
      if(entity->flags & EntityType_Rotating) {
        entity->yaw += 30.0f * RadiansPerDegree * world->stopWatch.delta;
      }
    }
  }
}

void portalScene(GLFWwindow* window) {
  vec2_u32 windowExtent = getWindowExtent();
  const vec2_u32 initWindowExtent = windowExtent;
  f32 aspectRatio = f32(windowExtent.width) / windowExtent.height;

  World world{};

  VertexAtt cubePosVertexAtt = cubePosVertexAttBuffers();
  portalVertexAtt = quadPosVertexAttBuffers(false);
  portalBackingBoxVertexAtt = cubePosVertexAttBuffers(true, true);

  ShaderProgram albedoNormalTexShader = createShaderProgram(gateVertexShaderFileLoc, gateFragmentShaderFileLoc);
  ShaderProgram singleColorShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);
  ShaderProgram reflectSkyboxShader = createShaderProgram(posNormVertexShaderFileLoc, reflectSkyboxFragmentShaderFileLoc);
  stencilShader = createShaderProgram(posVertexShaderFileLoc, blackFragmentShaderFileLoc);

  ProjectionViewModelUBO projectionViewModelUbo;

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

  vec3 gatePortalNegativeXCenter = (cubeFaceNegativeXCenter * gateScale.x) + gatePosition;
  vec3 gatePortalPositiveXCenter = (cubeFacePositiveXCenter * gateScale.x) + gatePosition;
  vec3 gatePortalNegativeYCenter = (cubeFaceNegativeYCenter * gateScale.y) + gatePosition;
  vec3 gatePortalPositiveYCenter = (cubeFacePositiveYCenter * gateScale.y) + gatePosition;

  u32 skyboxModelIndex = addNewModel_Skybox(&world);

  world.currentSceneIndex = gateSceneIndex;
  u32 gateEntityIndex;
  {
    u32 gateModelIndex = addNewModel(&world, gateModelLoc);
    gateEntityIndex = addNewEntity(&world, gateSceneIndex, gateModelIndex, gatePosition, gateScale, gateYaw, albedoNormalTexShader);

    loadCubeMapTexture(caveFaceLocations, &world.scenes[gateSceneIndex].skyboxTexture);
    u32 caveSkyboxIndex = addNewEntity(&world, gateSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&world, gateEntityIndex, gatePortalNegativeXCenter, negativeXNormal, portalDimens, destPaperStencilMask, paperSceneIndex);
    addPortal(&world, gateEntityIndex, gatePortalPositiveXCenter, positiveXNormal, portalDimens, destOctahedronStencilMask, octahedronSceneIndex);
    addPortal(&world, gateEntityIndex, gatePortalNegativeYCenter, negativeYNormal, portalDimens, destTetrahedronStencilMask, tetrahedronSceneIndex);
    addPortal(&world, gateEntityIndex, gatePortalPositiveYCenter, positiveYNormal, portalDimens, destIcosahedronStencilMask, icosahedronSceneIndex);

    gateSceneFragUbo.directionalLightColor = {0.5f, 0.5f, 0.5f};
    gateSceneFragUbo.ambientLightColor = {0.2f, 0.2f, 0.2f};
    gateSceneFragUbo.directionalLightDirToSource = {1.0f, 1.0f, 1.0f};
    u32 portalFragUBOBindingIndex = 1;
    GLuint gateSceneFragUBOid;
    glGenBuffers(1, &gateSceneFragUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, gateSceneFragUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(gateSceneFragUbo), &gateSceneFragUbo, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, portalFragUBOBindingIndex, gateSceneFragUBOid, 0, sizeof(gateSceneFragUbo));
  }

  {
    u32 tetrahedronModelIndex = addNewModel(&world, tetrahedronModelLoc);
    u32 tetrahedronEntityIndex = addNewEntity(&world, tetrahedronSceneIndex, tetrahedronModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    world.models[tetrahedronModelIndex].meshes[0].textureData.baseColor = vec4{1.0f, 0.4f, 0.4f, 1.0f};

    loadCubeMapTexture(yellowCloudFaceLocations, &world.scenes[tetrahedronSceneIndex].skyboxTexture);
    u32 yellowCloudSkyboxIndex = addNewEntity(&world, tetrahedronSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&world, tetrahedronSceneIndex, gatePortalNegativeYCenter, positiveYNormal, portalDimens, destGateStencilMask, gateEntityIndex);
  }

  {
    u32 octahedronModelIndex = addNewModel(&world, octahedronModelLoc);
    u32 octahedronEntityIndex = addNewEntity(&world, octahedronSceneIndex, octahedronModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    world.models[octahedronModelIndex].meshes[0].textureData.baseColor = vec4{0.4f, 1.0f, 0.4f, 1.0f};

    loadCubeMapTexture(skyboxInterstellarFaceLocations, &world.scenes[octahedronSceneIndex].skyboxTexture);
    u32 interstellarSkyboxIndex = addNewEntity(&world, octahedronSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&world, octahedronSceneIndex, gatePortalPositiveXCenter, negativeXNormal, portalDimens, destGateStencilMask, gateEntityIndex);
  }

  {
    u32 paperModelIndex = addNewModel(&world, paperModelLoc);
    u32 paperEntityIndex = addNewEntity(&world, paperSceneIndex, paperModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    world.models[paperModelIndex].meshes[0].textureData.baseColor = vec4{0.4f, 0.4f, 1.0f, 1.0f};

    loadCubeMapTexture(calmSeaFaceLocations, &world.scenes[paperSceneIndex].skyboxTexture);
    u32 calmSeaSkyboxIndex = addNewEntity(&world, paperSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&world, paperSceneIndex, gatePortalNegativeXCenter, positiveXNormal, portalDimens, destGateStencilMask, gateEntityIndex);
  }

  {
    u32 icosahedronModelIndex = addNewModel(&world, icosahedronModelLoc);
    u32 icosahedronEntityIndex = addNewEntity(&world, icosahedronSceneIndex, icosahedronModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    world.models[icosahedronModelIndex].meshes[0].textureData.baseColor = vec4{0.9f, 0.9f, 0.9f, 1.0f};

    loadCubeMapTexture(pollutedEarthFaceLocations, &world.scenes[icosahedronSceneIndex].skyboxTexture);
    u32 pollutedEarthSkyboxIndex = addNewEntity(&world, icosahedronSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&world, icosahedronSceneIndex, gatePortalPositiveYCenter, negativeYNormal, portalDimens, destGateStencilMask, gateEntityIndex);

    u32 portalBackingModelIndex = addNewModel(&world, portalBackingModelLoc);
    vec3 portalBackingModelScale = vec3{portalDimens.x, 0.5f, portalDimens.y};
    u32 portalSceneToGatePortalBackingEntity = addNewEntity(&world, icosahedronSceneIndex, portalBackingModelIndex, gatePortalPositiveYCenter, portalBackingModelScale, 0.0f, reflectSkyboxShader);
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

  // UBOs
  {
    glGenBuffers(1, &projViewModelGlobalUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewModelUBO), NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, projectionViewModelUBOBindingIndex, projViewModelGlobalUBOid, 0, sizeof(ProjectionViewModelUBO));
  }

  world.stopWatch = createStopWatch();
  while(glfwWindowShouldClose(window) == GL_FALSE)
  {
    loadInputStateForFrame(window);
    updateStopWatch(&world.stopWatch);

    vec3 playerCenter;
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
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
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
      playerCenter = calcBoundingBoxCenterPosition(world.player.boundingBox);

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
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    // TODO: don't need to set projection matrix right now but will in the future so keeping
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &projectionViewModelUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if(world.camera.thirdPerson) { // draw player if third person
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
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerCenter) * playerBoundingBoxScaleMatrix;
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerBoundingBoxColor_Red);
      drawTriangles(&cubePosVertexAtt);

      // debug player center
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerCenter) * scale_mat4(0.05f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerMinCoordBoxColor_Black);
      drawTriangles(&cubePosVertexAtt);

      // debug player min coordinate box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(world.player.boundingBox.min) * scale_mat4(0.1f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerMinCoordBoxColor_Green);
      drawTriangles(&cubePosVertexAtt);

      // debug player view
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = translate_mat4(playerViewCenter) * scale_mat4(0.1f);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerViewBoxColor_White);
      drawTriangles(&cubePosVertexAtt);

      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    updateEntities(&world);
    drawSceneWithPortals(&world);

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(albedoNormalTexShader);
  deleteShaderProgram(singleColorShader);
  deleteShaderProgram(skyboxShader);
}