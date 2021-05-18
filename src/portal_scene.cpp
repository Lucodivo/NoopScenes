#define PORTAL_BACKING_BOX_DEPTH 0.5f

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
  b32 flags; // EntityType flags
  vec3 position;
  vec3 scale;
  f32 yaw; // NOTE: Radians. 0 rads starts at {0, -1} and goes around the xy-plane in a CCW as seen from above
};

enum PortalState {
  PortalState_Visisble = 1 << 0,
  PortalState_InFocus = 1 << 1,
  PortalState_Entered = 1 << 2
};

struct Portal {
  vec3 normal;
  vec3 centerPosition;
  vec2 dimens;
  b32 flags; // PortalState flags
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
  f32 fov;
  f32 aspect;
  ProjectionViewModelUBO projectionViewModelUbo;
  FragUBO fragUbo;
};

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
const f32 near = 0.1f;
const f32 far = 200.0f;

const s32 skyboxActiveTextureIndex = 0;
const s32 albedoActiveTextureIndex = 1;
const s32 normalActiveTextureIndex = 2;
const s32 tiledNoiseActiveTextureIndex = 3;

global_variable GLuint projViewModelGlobalUBOid = 0;
global_variable GLuint fragUBOid = 0;
global_variable ShaderProgram stencilShader{};
global_variable VertexAtt portalQuadVertexAtt{};
global_variable VertexAtt portalBoxVertexAtt{};
global_variable World globalWorld{};

void drawScene(World* world, const u32 sceneIndex, u32 stencilMask = 0x00);
void drawPortals(World* world, const u32 sceneIndex);

void clearFlags(b32* flags) {
  *flags = 0;
}

void setFlags(b32* flags, b32 desiredFlags){
  *flags |= desiredFlags;
}

void overrideFlags(b32* flags, b32 desiredFlags) {
  *flags = desiredFlags;
}

b32 flagIsSet(b32 flags, b32 checkFlag) {
  return flags & checkFlag;
}

b32 flagsAreSet(b32 flags, b32 checkFlags) {
  return (flags & checkFlags) == checkFlags;
}

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
  portal.flags = 0;

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
  entity->yaw = yaw;
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

struct GateSceneFragUBO {
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

mat4 calcBoxStencilModelMatFromPortalModelMat(const mat4& portalModelMat) {
  return portalModelMat * scale_mat4(vec3{1.0f, PORTAL_BACKING_BOX_DEPTH, 1.0f}) * translate_mat4(-cubeFaceNegativeYCenter);
}

void drawPortal(const World* world, Portal* portal) {
  if(!flagIsSet(portal->flags, PortalState_Visisble)) { return; }

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

  mat4 portalModelMat = quadModelMatrix(portal->centerPosition, portal->normal, portal->dimens.x, portal->dimens.y);
  VertexAtt* portalVertexAtt = &portalQuadVertexAtt;
  if(flagIsSet(portal->flags, PortalState_InFocus)) {
    portalModelMat = calcBoxStencilModelMatFromPortalModelMat(portalModelMat);
    portalVertexAtt = &portalBoxVertexAtt;
  }

  glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
  glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portalModelMat);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glUseProgram(stencilShader.id);
  glStencilMask(portal->stencilMask);
  drawTriangles(portalVertexAtt);
}

void drawPortals(World* world, const u32 sceneIndex){

  Scene* scene = world->scenes + sceneIndex;

  for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
    drawPortal(world, scene->portals + portalIndex);
  }

  // turn off writes to the stencil
  glStencilMask(0x00);

  // Draw portal scenes
  // We need to clear disable depth values so distant objects through the "portals" still get drawn
  // The portals themselves will still obey the depth of the scene, as the stencils have been rendered with depth in mind
  glClear(GL_DEPTH_BUFFER_BIT);

  for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
    Portal portal = scene->portals[portalIndex];

    vec3 portalNormal_viewSpace = (world->projectionViewModelUbo.view * Vec4(-portal.normal, 0.0f)).xyz;
    vec3 portalCenterPos_viewSpace = (world->projectionViewModelUbo.view * Vec4(portal.centerPosition, 1.0f)).xyz;
    mat4 portalProjectionMat = obliquePerspective(world->fov, world->aspect, near, far, portalNormal_viewSpace, portalCenterPos_viewSpace);

    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, projection), sizeof(mat4), &portalProjectionMat);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    drawScene(world, portal.sceneDestination, portal.stencilMask);
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

    mat4 modelMatrix = scaleRotTrans_mat4(entity->scale, vec3{0.0f, 0.0f, 1.0f}, entity->yaw, entity->position);

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

  Scene* currentScene = &world->scenes[world->currentSceneIndex];
  vec3 playerViewPosition = calcPlayerViewingPosition(&world->player);
  b32 portalEntered = false;
  u32 portalSceneDestination;
  auto updatePortalsForScene = [playerViewPosition, &portalEntered, &portalSceneDestination](Scene* scene) {
    // TODO: optimize only checking what we need to for portals
    for(u32 portalIndex = 0; portalIndex < scene->portalCount; ++portalIndex) {
      Portal* portal = scene->portals + portalIndex;

      vec3 portalCenterToPlayerView = playerViewPosition - portal->centerPosition;
      b32 viewerInFrontOfPortal = similarDirection(portal->normal, playerViewPosition - portal->centerPosition) ? PortalState_Visisble : false;

      b32 portalWasInFocus = portal->flags & PortalState_InFocus;
      vec3 viewPositionPerpendicularToPortal = perpendicularTo(portalCenterToPlayerView, portal->normal);
      f32 widthDistFromCenter = magnitude(viewPositionPerpendicularToPortal.xy);
      f32 heightDistFromCenter = viewPositionPerpendicularToPortal.z;
      b32 viewerInsideDimens = widthDistFromCenter < (portal->dimens.x * 0.5f) && heightDistFromCenter < (portal->dimens.y * 0.5f);

      b32 portalInFocus = (viewerInFrontOfPortal && viewerInsideDimens) ? PortalState_InFocus : false;
      b32 insidePortal = (portalWasInFocus && !viewerInFrontOfPortal) ? PortalState_Entered : false; // portal was in focus and now we're on the other side

      overrideFlags(&portal->flags, viewerInFrontOfPortal | portalInFocus | insidePortal);

      if(insidePortal){
        portalEntered = true;
        portalSceneDestination = portal->sceneDestination;
      }
    }
  };

  updatePortalsForScene(currentScene);
  if(portalEntered) {
    // clear portals for old scene
    for(u32 portalIndex = 0; portalIndex < currentScene->portalCount; ++portalIndex) {
      clearFlags(&currentScene->portals[portalIndex].flags); // clear all flags of previous currentScene
    }

    world->currentSceneIndex = portalSceneDestination;

    // update portals for new scene
    updatePortalsForScene(world->scenes + world->currentSceneIndex);
  }
}

void portalScene(GLFWwindow* window) {
  vec2_u32 windowExtent = getWindowExtent();
  const vec2_u32 initWindowExtent = windowExtent;
  globalWorld.aspect = f32(windowExtent.width) / windowExtent.height;

  VertexAtt cubePosVertexAtt = cubePosVertexAttBuffers();
  portalQuadVertexAtt = quadPosVertexAttBuffers(false);
  portalBoxVertexAtt = cubePosVertexAttBuffers(true, true);

  ShaderProgram albedoNormalTexShader = createShaderProgram(gateVertexShaderFileLoc, gateFragmentShaderFileLoc);
  ShaderProgram singleColorShader = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  ShaderProgram skyboxShader = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);
  ShaderProgram reflectSkyboxShader = createShaderProgram(posNormVertexShaderFileLoc, reflectSkyboxFragmentShaderFileLoc);
  stencilShader = createShaderProgram(posVertexShaderFileLoc, blackFragmentShaderFileLoc);

  globalWorld.player.boundingBox.diagonal = defaultPlayerDimensionInMeters;
  globalWorld.player.boundingBox.min = {-(globalWorld.player.boundingBox.diagonal.x * 0.5f), -10.0f - (globalWorld.player.boundingBox.diagonal.y * 0.5f), 0.0f};

  vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&globalWorld.player);
  vec3 firstPersonCameraInitFocus{gatePosition.x, gatePosition.y, firstPersonCameraInitPosition.z};
  lookAt_FirstPerson(firstPersonCameraInitPosition, firstPersonCameraInitFocus, &globalWorld.camera);

  globalWorld.fov = fieldOfView(13.5f, 25.0f);
  globalWorld.projectionViewModelUbo.projection = perspective(globalWorld.fov, globalWorld.aspect, near, far);

  u32 gateSceneIndex = addNewScene(&globalWorld);
  u32 tetrahedronSceneIndex = addNewScene(&globalWorld);
  u32 octahedronSceneIndex = addNewScene(&globalWorld);
  u32 paperSceneIndex = addNewScene(&globalWorld);
  u32 icosahedronSceneIndex = addNewScene(&globalWorld);
  GLuint gateSceneFragUBOid;

  vec3 gatePortalNegativeXCenter = (cubeFaceNegativeXCenter * gateScale.x) + gatePosition;
  vec3 gatePortalPositiveXCenter = (cubeFacePositiveXCenter * gateScale.x) + gatePosition;
  vec3 gatePortalNegativeYCenter = (cubeFaceNegativeYCenter * gateScale.y) + gatePosition;
  vec3 gatePortalPositiveYCenter = (cubeFacePositiveYCenter * gateScale.y) + gatePosition;

  u32 skyboxModelIndex = addNewModel_Skybox(&globalWorld);
  u32 portalBackingModelIndex = addNewModel(&globalWorld, portalBackingModelLoc);
  vec3 portalBackingModelScale = vec3{portalDimens.x, 0.5f, portalDimens.y};

  globalWorld.currentSceneIndex = gateSceneIndex;
  u32 gateEntityIndex;
  {
    u32 gateModelIndex = addNewModel(&globalWorld, gateModelLoc);
    gateEntityIndex = addNewEntity(&globalWorld, gateSceneIndex, gateModelIndex, gatePosition, gateScale, gateYaw, albedoNormalTexShader);

    loadCubeMapTexture(caveFaceLocations, &globalWorld.scenes[gateSceneIndex].skyboxTexture);
    u32 caveSkyboxIndex = addNewEntity(&globalWorld, gateSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&globalWorld, gateEntityIndex, gatePortalNegativeXCenter, negativeXNormal, portalDimens, destPaperStencilMask, paperSceneIndex);
    addPortal(&globalWorld, gateEntityIndex, gatePortalPositiveXCenter, positiveXNormal, portalDimens, destOctahedronStencilMask, octahedronSceneIndex);
    addPortal(&globalWorld, gateEntityIndex, gatePortalNegativeYCenter, negativeYNormal, portalDimens, destTetrahedronStencilMask, tetrahedronSceneIndex);
    addPortal(&globalWorld, gateEntityIndex, gatePortalPositiveYCenter, positiveYNormal, portalDimens, destIcosahedronStencilMask, icosahedronSceneIndex);

    gateSceneFragUbo.directionalLightColor = {0.5f, 0.5f, 0.5f};
    gateSceneFragUbo.ambientLightColor = {0.3f, 0.3f, 0.3f};
    gateSceneFragUbo.directionalLightDirToSource = {1.0f, 1.0f, 1.0f};
  }

  {
    u32 tetrahedronModelIndex = addNewModel(&globalWorld, tetrahedronModelLoc);
    u32 tetrahedronEntityIndex = addNewEntity(&globalWorld, tetrahedronSceneIndex, tetrahedronModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    globalWorld.models[tetrahedronModelIndex].meshes[0].textureData.baseColor = vec4{1.0f, 0.4f, 0.4f, 1.0f};

    loadCubeMapTexture(yellowCloudFaceLocations, &globalWorld.scenes[tetrahedronSceneIndex].skyboxTexture);
    u32 yellowCloudSkyboxIndex = addNewEntity(&globalWorld, tetrahedronSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&globalWorld, tetrahedronSceneIndex, gatePortalNegativeYCenter, positiveYNormal, portalDimens, destGateStencilMask, gateEntityIndex);

    vec3 portalBackingPosition = gatePortalNegativeYCenter;
    portalBackingPosition += vec3{0.0f, portalBackingModelScale.y * -0.5f, 0.0f};
    u32 portalSceneToGatePortalBackingEntity = addNewEntity(&globalWorld, tetrahedronSceneIndex, portalBackingModelIndex,
                                                            portalBackingPosition, portalBackingModelScale, 180.0f * RadiansPerDegree, reflectSkyboxShader);
  }

  {
    u32 octahedronModelIndex = addNewModel(&globalWorld, octahedronModelLoc);
    u32 octahedronEntityIndex = addNewEntity(&globalWorld, octahedronSceneIndex, octahedronModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    globalWorld.models[octahedronModelIndex].meshes[0].textureData.baseColor = vec4{0.4f, 1.0f, 0.4f, 1.0f};

    loadCubeMapTexture(skyboxInterstellarFaceLocations, &globalWorld.scenes[octahedronSceneIndex].skyboxTexture);
    u32 interstellarSkyboxIndex = addNewEntity(&globalWorld, octahedronSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&globalWorld, octahedronSceneIndex, gatePortalPositiveXCenter, negativeXNormal, portalDimens, destGateStencilMask, gateEntityIndex);

    vec3 portalBackingPosition = gatePortalPositiveXCenter;
    portalBackingPosition += vec3{portalBackingModelScale.y * 0.5f, 0.0f, 0.0f};
    u32 portalSceneToGatePortalBackingEntity = addNewEntity(&globalWorld, octahedronSceneIndex, portalBackingModelIndex,
                                                            portalBackingPosition, portalBackingModelScale, -90.0f * RadiansPerDegree, reflectSkyboxShader);
  }

  {
    u32 paperModelIndex = addNewModel(&globalWorld, paperModelLoc);
    u32 paperEntityIndex = addNewEntity(&globalWorld, paperSceneIndex, paperModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    globalWorld.models[paperModelIndex].meshes[0].textureData.baseColor = vec4{0.4f, 0.4f, 1.0f, 1.0f};

    loadCubeMapTexture(calmSeaFaceLocations, &globalWorld.scenes[paperSceneIndex].skyboxTexture);
    u32 calmSeaSkyboxIndex = addNewEntity(&globalWorld, paperSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&globalWorld, paperSceneIndex, gatePortalNegativeXCenter, positiveXNormal, portalDimens, destGateStencilMask, gateEntityIndex);

    vec3 portalBackingPosition = gatePortalNegativeXCenter;
    portalBackingPosition += vec3{portalBackingModelScale.y * -0.5f, 0.0f, 0.0f};
    u32 portalSceneToGatePortalBackingEntity = addNewEntity(&globalWorld, paperSceneIndex, portalBackingModelIndex,
                                                            portalBackingPosition, portalBackingModelScale, 90.0f * RadiansPerDegree, reflectSkyboxShader);
  }

  {
    u32 icosahedronModelIndex = addNewModel(&globalWorld, icosahedronModelLoc);
    u32 icosahedronEntityIndex = addNewEntity(&globalWorld, icosahedronSceneIndex, icosahedronModelIndex, shapePosition, shapeScale, 0.0f, singleColorShader, EntityType_Rotating | EntityType_Wireframe);
    globalWorld.models[icosahedronModelIndex].meshes[0].textureData.baseColor = vec4{0.9f, 0.9f, 0.9f, 1.0f};

    loadCubeMapTexture(pollutedEarthFaceLocations, &globalWorld.scenes[icosahedronSceneIndex].skyboxTexture);
    u32 pollutedEarthSkyboxIndex = addNewEntity(&globalWorld, icosahedronSceneIndex, skyboxModelIndex, skyboxPosition, skyboxScale, 0.0f, skyboxShader);

    addPortal(&globalWorld, icosahedronSceneIndex, gatePortalPositiveYCenter, negativeYNormal, portalDimens, destGateStencilMask, gateEntityIndex);

    vec3 portalBackingPosition = gatePortalPositiveYCenter;
    portalBackingPosition += vec3{0.0f, portalBackingModelScale.y * 0.5f, 0.0f};
    u32 portalSceneToGatePortalBackingEntity = addNewEntity(&globalWorld, icosahedronSceneIndex, portalBackingModelIndex,
                                                            portalBackingPosition, portalBackingModelScale, 0.0f, reflectSkyboxShader);
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

  u32 tiledTextureId;
  load2DTexture(tiledDisplacement1TextureFileLoc, &tiledTextureId);
  glUseProgram(albedoNormalTexShader.id);
  bindActiveTextureSampler2d(tiledNoiseActiveTextureIndex, tiledTextureId);
  setSampler2D(albedoNormalTexShader.id, "tiledNoiseTex", tiledNoiseActiveTextureIndex);

  // UBOs
  {
    glGenBuffers(1, &projViewModelGlobalUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewModelUBO), NULL, GL_STREAM_DRAW);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, projectionViewModelUBOBindingIndex, projViewModelGlobalUBOid, 0, sizeof(ProjectionViewModelUBO));

    u32 fragUBOBindingIndex = 1;
    glGenBuffers(1, &fragUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, fragUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FragUBO), NULL, GL_STREAM_DRAW);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, fragUBOBindingIndex, fragUBOid, 0, sizeof(FragUBO));

    u32 portalFragUBOBindingIndex = 2;
    glGenBuffers(1, &gateSceneFragUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, gateSceneFragUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(gateSceneFragUbo), &gateSceneFragUbo, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, portalFragUBOBindingIndex, gateSceneFragUBOid, 0, sizeof(gateSceneFragUbo));
  }

  globalWorld.stopWatch = createStopWatch();
  while(glfwWindowShouldClose(window) == GL_FALSE)
  {
    loadInputStateForFrame(window);
    updateStopWatch(&globalWorld.stopWatch);
    globalWorld.fragUbo.time = globalWorld.stopWatch.totalElapsed;

    vec3 playerCenter;
    vec3 playerViewPosition = calcPlayerViewingPosition(&globalWorld.player);

    if (isActive(KeyboardInput_Esc))
    {
      glfwSetWindowShouldClose(window, true);
      break;
    }

    // toggle fullscreen/window mode if alt + enter
    if(isActive(KeyboardInput_Alt_Right) && hotPress(KeyboardInput_Enter)) {
      windowExtent = toggleWindowSize(window, initWindowExtent.width, initWindowExtent.height);
      globalWorld.aspect = f32(windowExtent.width) / windowExtent.height;
      glViewport(0, 0, windowExtent.width, windowExtent.height);

      adjustAspectPerspProj(&globalWorld.projectionViewModelUbo.projection, globalWorld.fov, globalWorld.aspect);
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
        f32 playerMovementSpeed = leftShiftIsActive ? 8.0f : 4.0f;

        // Camera movement direction
        vec3 playerMovementDirection{};
        if (lateralMovement)
        {
          playerMovementDirection += rightIsActive ? globalWorld.camera.right : -globalWorld.camera.right;
        }

        if (forwardMovement)
        {
          playerMovementDirection += upIsActive ? globalWorld.camera.forward : -globalWorld.camera.forward;
        }

        playerMovementDirection = normalize(playerMovementDirection.x, playerMovementDirection.y, 0.0);
        playerDelta = playerMovementDirection * playerMovementSpeed * globalWorld.stopWatch.delta;
      }

      // TODO: Do not apply immediately, check for collisions
      globalWorld.player.boundingBox.min += playerDelta;
      playerCenter = calcBoundingBoxCenterPosition(globalWorld.player.boundingBox);

      if(tabHotPress) { // switch between third and first person
        vec3 xyForward = normalize(globalWorld.camera.forward.x, globalWorld.camera.forward.y, 0.0f);

        if(!globalWorld.camera.thirdPerson) {
          lookAt_ThirdPerson(playerCenter, xyForward, &globalWorld.camera);
        } else { // camera is first person now
          vec3 focus = playerViewPosition + xyForward;
          lookAt_FirstPerson(playerViewPosition, focus, &globalWorld.camera);
        }
      }

      const f32 mouseDeltaMultConst = 0.0005f;
      if(globalWorld.camera.thirdPerson) {
        updateCamera_ThirdPerson(&globalWorld.camera, playerCenter, f32(-mouseDelta.y * mouseDeltaMultConst),f32(-mouseDelta.x * mouseDeltaMultConst));
      } else {
        updateCamera_FirstPerson(&globalWorld.camera, playerDelta, f32(-mouseDelta.y * mouseDeltaMultConst), f32(-mouseDelta.x * mouseDeltaMultConst));
      }

      globalWorld.projectionViewModelUbo.view = getViewMat(globalWorld.camera);
    }

    // draw
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, // stencil function always passes
                  0x00, // reference
                  0x00); // mask
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // universal matrices in UBO
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, offsetof(ProjectionViewModelUBO, model), &globalWorld.projectionViewModelUbo);

    glBindBuffer(GL_UNIFORM_BUFFER, fragUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FragUBO), &globalWorld.fragUbo);

    glBindBuffer(GL_UNIFORM_BUFFER, gateSceneFragUBOid);
    f32 scaleElapsedTime = globalWorld.stopWatch.totalElapsed * 0.2;
    gateSceneFragUbo.directionalLightDirToSource = normalize(vec3{cosf(scaleElapsedTime) * 3.0f, sinf(scaleElapsedTime) * 3.0f, 1.0f});
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(GateSceneFragUBO, directionalLightDirToSource), sizeof(vec3), &gateSceneFragUbo.directionalLightDirToSource);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);



    if(globalWorld.camera.thirdPerson) { // draw player if third person
      vec3 playerViewCenter = calcPlayerViewingPosition(&globalWorld.player);
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
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(globalWorld.player.boundingBox.diagonal, playerCenter);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      setUniform(singleColorShader.id, "baseColor", playerBoundingBoxColor_Red);
      drawTriangles(&cubePosVertexAtt);

      // debug player center
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(0.05f, playerCenter);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      setUniform(singleColorShader.id, "baseColor", playerMinCoordBoxColor_Black);
      drawTriangles(&cubePosVertexAtt);

      // debug player min coordinate box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(0.1f, globalWorld.player.boundingBox.min);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      setUniform(singleColorShader.id, "baseColor", playerMinCoordBoxColor_Green);
      drawTriangles(&cubePosVertexAtt);

      // debug player view
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(0.1f, playerViewCenter);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(singleColorShader.id, "baseColor", playerViewBoxColor_White);
      drawTriangles(&cubePosVertexAtt);

      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    updateEntities(&globalWorld);
    drawSceneWithPortals(&globalWorld);

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderProgram(albedoNormalTexShader);
  deleteShaderProgram(singleColorShader);
  deleteShaderProgram(skyboxShader);
}