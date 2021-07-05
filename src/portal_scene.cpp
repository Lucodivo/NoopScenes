#define PORTAL_BACKING_BOX_DEPTH 0.5f
#define MAX_PORTALS 4

struct Player {
  BoundingBox boundingBox;
};

enum EntityType {
  EntityType_Rotating = 1 << 0,
  EntityType_Wireframe = 1 << 1
};

struct Entity {
  BoundingBox boundingBox;
  u32 modelIndex;
  u32 shaderIndex;
  b32 typeFlags; // EntityType flags
  vec3 position;
  vec3 scale;
  f32 yaw; // NOTE: Radians. 0 rads starts at {0, -1} and goes around the xy-plane in a CCW as seen from above
};

enum PortalState {
  PortalState_FacingCamera = 1 << 0,
  PortalState_InFocus = 1 << 1
};

struct Portal {
  vec3 normal;
  vec3 centerPosition;
  vec2 dimens;
  b32 stateFlags; // PortalState flags
  u32 stencilMask;
  u32 sceneDestination;
};

struct Scene {
  // TODO: should the scene keep track of its own index in the world?
  Entity entities[16];
  u32 entityCount;
  Portal portals[MAX_PORTALS];
  u32 portalCount;
  GLuint skyboxTexture;
  const char* title;
  const char* skyboxDir;
  const char* skyboxExt;
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
  ShaderProgram shaders[16];
  u32 shaderCount;
};

const vec3 defaultPlayerDimensionInMeters{0.5f, 0.25f, 1.75f}; // NOTE: ~1'7"w, 9"d, 6'h
const f32 near = 0.1f;
const f32 far = 200.0f;

global_variable GLuint projViewModelGlobalUBOid = 0;
global_variable GLuint fragUBOid = 0;
global_variable GLuint portalQueryObjects[MAX_PORTALS];
global_variable World globalWorld{};

global_variable struct {
  VertexAtt portalQuad{};
  VertexAtt portalBox{};
  VertexAtt skyboxBox{};
} globalVertexAtts;

global_variable union {
  struct {
    ShaderProgram singleColor;
    ShaderProgram skybox;
    ShaderProgram stencil;
  };
  ShaderProgram shaders[3];
} globalShaders;

void drawScene(World* world, const u32 sceneIndex, u32 stencilMask = 0x00);
void drawPortals(World* world, const u32 sceneIndex);

void addPortal(World* world, u32 homeSceneIndex,
               const vec3& centerPosition, const vec3& normal, const vec2& dimens,
               const u32 stencilMask, const u32 destinationSceneIndex) {
  Assert(stencilMask <= MAX_STENCIL_VALUE && stencilMask > 0);

  Scene* homeScene = world->scenes + homeSceneIndex;
  Assert(ArrayCount(homeScene->portals) > homeScene->portalCount);

  Portal portal{};
  portal.stencilMask = stencilMask;
  portal.dimens = dimens;
  portal.centerPosition = centerPosition;
  portal.normal = normal;
  portal.sceneDestination = destinationSceneIndex;
  portal.stateFlags = 0;

  homeScene->portals[homeScene->portalCount++] = portal;
}

u32 addNewScene(World* world, const char* title) {
  Assert(ArrayCount(world->scenes) > world->sceneCount);
  u32 sceneIndex = world->sceneCount++;
  Scene* scene = world->scenes + sceneIndex;
  *scene = {};
  scene->title = title;
  return sceneIndex;
}

u32 addNewShader(World* world, const char* vertexShaderFileLoc, const char* fragmentShaderFileLoc, const char* noiseTexture = nullptr) {
  Assert(ArrayCount(world->shaders) > world->shaderCount);
  u32 shaderIndex = world->shaderCount++;
  ShaderProgram* shader = world->shaders + shaderIndex;
  *shader = createShaderProgram(vertexShaderFileLoc, fragmentShaderFileLoc, noiseTexture);
  return shaderIndex;
}

u32 addNewEntity(World* world, u32 sceneIndex, u32 modelIndex,
                 vec3 pos, vec3 scale, f32 yaw,
                 u32 shaderIndex, b32 entityTypeFlags = 0) {
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
  entity->shaderIndex = shaderIndex;
  entity->typeFlags = entityTypeFlags;
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

inline vec3 calcBoundingBoxCenterPosition(BoundingBox box) {
  return box.min + (box.diagonal * 0.5f);
}

// assume "eyes" (FPS camera) is positioned on middle X, max Y, max Z
inline vec3 calcPlayerViewingPosition(const Player* player) {
  return player->boundingBox.min + hadamard(player->boundingBox.diagonal, {0.5f, 1.0f, 1.0f});
}

void drawTrianglesWireframe(const VertexAtt* vertexAtt) {
  // TODO: This will not work at all in a general case
  // TODO: It only currently works because the wireframe shapes are the first things we draw in each scene
  // TODO: can't just disable the depth test whenever
  glUseProgram(globalShaders.singleColor.id);
  setUniform(globalShaders.singleColor.id, "baseColor", vec3{0.0f, 0.0f, 0.0f});
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
  glUseProgram(globalShaders.stencil.id);

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
  VertexAtt* portalVertexAtt = &globalVertexAtts.portalQuad;
  if(flagIsSet(portal->stateFlags, PortalState_InFocus)) {
    portalModelMat = calcBoxStencilModelMatFromPortalModelMat(portalModelMat);
    portalVertexAtt = &globalVertexAtts.portalBox;
  }

  glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
  glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &portalModelMat);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glUseProgram(globalShaders.stencil.id);
  glStencilMask(portal->stencilMask);
  drawTriangles(portalVertexAtt);
}

void drawPortals(World* world, const u32 sceneIndex){

  Scene* scene = world->scenes + sceneIndex;

  for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
    Portal* portal = scene->portals + portalIndex;
    // don't draw portals if portal isn't visible
    // TODO: better visibility tests besides facing camera?
    if(!flagIsSet(portal->stateFlags, PortalState_FacingCamera)) { continue; }

    // begin occlusion query
    glBeginQuery(GL_ANY_SAMPLES_PASSED, portalQueryObjects[portalIndex]);
    drawPortal(world, portal);
    // end occlusion query
    glEndQuery(GL_ANY_SAMPLES_PASSED);
  }

  // turn off writes to the stencil
  glStencilMask(0x00);

  // Draw portal scenes
  // We need to clear disable depth values so distant objects through the "portals" still get drawn
  // The portals themselves will still obey the depth of the scene, as the stencils have been rendered with depth in mind
  glClear(GL_DEPTH_BUFFER_BIT);

  for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
    Portal portal = scene->portals[portalIndex];
    // don't draw scene if portal isn't visible
    // TODO: better visibility tests besides facing camera?
    if(!flagIsSet(portal.stateFlags, PortalState_FacingCamera)) { continue; }

    vec3 portalNormal_viewSpace = (world->projectionViewModelUbo.view * Vec4(-portal.normal, 0.0f)).xyz;
    vec3 portalCenterPos_viewSpace = (world->projectionViewModelUbo.view * Vec4(portal.centerPosition, 1.0f)).xyz;
    mat4 portalProjectionMat = obliquePerspective(world->fov, world->aspect, near, far, portalNormal_viewSpace, portalCenterPos_viewSpace);

    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, projection), sizeof(mat4), &portalProjectionMat);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Conditional render only if the any samples passed while drawing the portal
    glBeginConditionalRender(portalQueryObjects[portalIndex], GL_QUERY_BY_REGION_WAIT);
    drawScene(world, portal.sceneDestination, portal.stencilMask);
    glEndConditionalRender();
  }
}

void drawScene(World* world, const u32 sceneIndex, u32 stencilMask) {
  glStencilFunc(
          GL_EQUAL, // test function applied to stored stencil value and ref [ex: discard when stored value GL_GREATER ref]
          stencilMask, // ref
          0xFF); // enable which bits in reference and stored value are compared

  Scene* scene = world->scenes + sceneIndex;

  if(scene->skyboxTexture != TEXTURE_ID_NO_TEXTURE) { // draw skybox if one exists
    glUseProgram(globalShaders.skybox.id);
    bindActiveTextureCubeMap(skyboxActiveTextureIndex, scene->skyboxTexture);
    setSamplerCube(globalShaders.skybox.id, skyboxTexUniformName, skyboxActiveTextureIndex);
    mat4 identityMat4 = identity_mat4();
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &identityMat4);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    drawTriangles(&globalVertexAtts.skyboxBox);
  }

  for(u32 sceneEntityIndex = 0; sceneEntityIndex < scene->entityCount; ++sceneEntityIndex) {
    Entity* entity = &scene->entities[sceneEntityIndex];
    ShaderProgram shader = world->shaders[entity->shaderIndex];

    mat4 modelMatrix = scaleRotTrans_mat4(entity->scale, vec3{0.0f, 0.0f, 1.0f}, entity->yaw, entity->position);

    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &modelMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glUseProgram(shader.id);
    if(shader.noiseTextureId != TEXTURE_ID_NO_TEXTURE) {
      bindActiveTextureSampler2d(noiseActiveTextureIndex, shader.noiseTextureId);
      setSampler2D(shader.id, noiseTexUniformName, noiseActiveTextureIndex);
    }
    Model model = world->models[entity->modelIndex];
    // TODO: Should some of this logic be moved to drawModel()?
    for(u32 meshIndex = 0; meshIndex < model.meshCount; ++meshIndex) {
      Mesh* mesh = model.meshes + meshIndex;
      if(mesh->textureData.baseColor.a != 0.0f) {
        setUniform(shader.id, baseColorUniformName, mesh->textureData.baseColor.rgb);
      }
      if(mesh->textureData.albedoTextureId != TEXTURE_ID_NO_TEXTURE) {
        bindActiveTextureSampler2d(albedoActiveTextureIndex, mesh->textureData.albedoTextureId);
        setSampler2D(shader.id, albedoTexUniformName, albedoActiveTextureIndex);
      }
      if(mesh->textureData.normalTextureId != TEXTURE_ID_NO_TEXTURE) {
        bindActiveTextureSampler2d(normalActiveTextureIndex, mesh->textureData.normalTextureId);
        setSampler2D(shader.id, normalTexUniformName, normalActiveTextureIndex);
      }

      drawTriangles(&mesh->vertexAtt);
    }

    if(entity->typeFlags & EntityType_Wireframe) { // wireframes should be drawn on top default mesh
      for(u32 meshIndex = 0; meshIndex < model.meshCount; ++meshIndex) {
        Mesh* mesh = model.meshes + meshIndex;
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
      if(entity->typeFlags & EntityType_Rotating) {
        entity->yaw += 30.0f * RadiansPerDegree * world->stopWatch.delta;
        if(entity->yaw > Tau32) {
          entity->yaw -= Tau32;
        }
      }
    }
  }

  Scene* currentScene = &world->scenes[world->currentSceneIndex];
  vec3 playerViewPosition = calcPlayerViewingPosition(&world->player);
  b32 portalEntered = false;
  u32 portalSceneDestination;
  auto updatePortalsForScene = [playerViewPosition, &portalEntered, &portalSceneDestination](Scene* scene) {
    for(u32 portalIndex = 0; portalIndex < scene->portalCount; ++portalIndex) {
      Portal* portal = scene->portals + portalIndex;

      vec3 portalCenterToPlayerView = playerViewPosition - portal->centerPosition;
      b32 portalFacingCamera = similarDirection(portal->normal, playerViewPosition - portal->centerPosition) ? PortalState_FacingCamera : false;

      b32 portalWasInFocus = flagIsSet(portal->stateFlags, PortalState_InFocus);
      vec3 viewPositionPerpendicularToPortal = perpendicularTo(portalCenterToPlayerView, portal->normal);
      f32 widthDistFromCenter = magnitude(viewPositionPerpendicularToPortal.xy);
      f32 heightDistFromCenter = viewPositionPerpendicularToPortal.z;
      b32 viewerInsideDimens = widthDistFromCenter < (portal->dimens.x * 0.5f) && heightDistFromCenter < (portal->dimens.y * 0.5f);

      b32 portalInFocus = (portalFacingCamera && viewerInsideDimens) ? PortalState_InFocus : false;
      b32 insidePortal = portalWasInFocus && !portalFacingCamera; // portal was in focus and now we're on the other side

      overrideFlags(&portal->stateFlags, portalFacingCamera | portalInFocus);

      if(insidePortal){
        portalEntered = true;
        portalSceneDestination = portal->sceneDestination;
        break;
      }
    }
  };

  // update portals for current scene
  updatePortalsForScene(currentScene);
  // if portal was entered, we need to update portals for new scene
  if(portalEntered) {
    // clear portals for old scene
    for(u32 portalIndex = 0; portalIndex < currentScene->portalCount; ++portalIndex) {
      clearFlags(&currentScene->portals[portalIndex].stateFlags); // clear all flags of previous currentScene
    }

    world->currentSceneIndex = portalSceneDestination;

    // update portals for new scene
    updatePortalsForScene(world->scenes + world->currentSceneIndex);
  }
}

void initGlobalShaders() {
  globalShaders.singleColor = createShaderProgram(posVertexShaderFileLoc, singleColorFragmentShaderFileLoc);
  globalShaders.stencil = createShaderProgram(posVertexShaderFileLoc, blackFragmentShaderFileLoc);
  globalShaders.skybox = createShaderProgram(skyboxVertexShaderFileLoc, skyboxFragmentShaderFileLoc);
}

void initGlobalVertexAtts() {
  globalVertexAtts.portalQuad = quadPosVertexAttBuffers(false);
  globalVertexAtts.portalBox = cubePosVertexAttBuffers(true, true);
  globalVertexAtts.skyboxBox = cubePosVertexAttBuffers(true);
}

void saveWorld(World* world, const char* title) {
  SaveFormat saveFormat{};

  saveFormat.startingSceneIndex = world->currentSceneIndex;

  saveFormat.scenes.reserve(world->sceneCount);
  saveFormat.shaders.reserve(world->shaderCount);
  saveFormat.models.reserve(world->modelCount);

  for(u32 modelIndex = 0; modelIndex < world->modelCount; modelIndex++) {
    Model* model = world->models + modelIndex;
    Assert(model->meshCount > 0);
    ModelSaveFormat modelSaveFormat{};
    modelSaveFormat.index = modelIndex;
    modelSaveFormat.baseColor = model->meshes[0].textureData.baseColor;
    modelSaveFormat.fileName = model->fileName;
    saveFormat.models.push_back(modelSaveFormat);
  }

  for(u32 shaderIndex = 0; shaderIndex < world->shaderCount; shaderIndex++) {
    ShaderProgram* shader = world->shaders + shaderIndex;
    ShaderSaveFormat shaderSaveFormat{};
    shaderSaveFormat.index = shaderIndex;
    shaderSaveFormat.vertexName = shader->vertexFileName;
    shaderSaveFormat.fragmentName = shader->fragmentFileName;
    if(shader->noiseTextureFileName != nullptr) {
      shaderSaveFormat.noiseTextureName = shader->noiseTextureFileName;
    } else {
      shaderSaveFormat.noiseTextureName.clear();
    }
    saveFormat.shaders.push_back(shaderSaveFormat);
  }

  for(u32 sceneIndex = 0; sceneIndex < world->sceneCount; sceneIndex++) {
    Scene* scene = world->scenes + sceneIndex;
    SceneSaveFormat sceneSaveFormat{};
    sceneSaveFormat.index = sceneIndex;
    sceneSaveFormat.title = scene->title;
    if(scene->skyboxTexture != TEXTURE_ID_NO_TEXTURE) {
      sceneSaveFormat.skyboxDir = scene->skyboxDir;
      sceneSaveFormat.skyboxExt = scene->skyboxExt;
    } else {
      sceneSaveFormat.skyboxDir.clear();
      sceneSaveFormat.skyboxExt.clear();
    }

    for(u32 entityIndex = 0; entityIndex < scene->entityCount; entityIndex++) {
      Entity* entity = scene->entities + entityIndex;
      EntitySaveFormat entitySaveFormat{};
      entitySaveFormat.shaderIndex = entity->shaderIndex;
      entitySaveFormat.modelIndex = entity->modelIndex;
      entitySaveFormat.scaleXYZ = entity->scale;
      entitySaveFormat.posXYZ = entity->position;
      entitySaveFormat.yaw = entity->yaw;
      entitySaveFormat.flags = entity->typeFlags;
      sceneSaveFormat.entities.push_back(entitySaveFormat);
    }

    for(u32 portalIndex = 0; portalIndex < scene->portalCount; portalIndex++) {
      Portal* portal = scene->portals + portalIndex;
      PortalSaveFormat portalSaveFormat{};
      portalSaveFormat.destination = portal->sceneDestination;
      portalSaveFormat.centerXYZ = portal->centerPosition;
      portalSaveFormat.normalXYZ = portal->normal;
      portalSaveFormat.dimensXY = portal->dimens;
      sceneSaveFormat.portals.push_back(portalSaveFormat);
    }

    saveFormat.scenes.push_back(sceneSaveFormat);
  }

  save(saveFormat, title);
}

void loadWorld(World* world, const char* saveJsonFile) {
  SaveFormat saveFormat = loadSave(saveJsonFile);

  size_t sceneCount = saveFormat.scenes.size();
  size_t modelCount = saveFormat.models.size();
  size_t shaderCount = saveFormat.shaders.size();

  u32* worldShaderIndices = new u32[shaderCount];
  { // shaders
    for(u32 shaderIndex = 0; shaderIndex < shaderCount; shaderIndex++) {
      ShaderSaveFormat shaderSaveFormat = saveFormat.shaders[shaderIndex];
      Assert(shaderSaveFormat.index < shaderCount);

      const char* noiseTexture = nullptr;
      if(!shaderSaveFormat.noiseTextureName.empty())
      {
        noiseTexture = shaderSaveFormat.noiseTextureName.c_str();
      }

      worldShaderIndices[shaderSaveFormat.index] = addNewShader(world, shaderSaveFormat.vertexName.c_str(), shaderSaveFormat.fragmentName.c_str(), noiseTexture);
    }
  }

  u32* worldModelIndices = new u32[modelCount];
  { // models
    for(u32 modelIndex = 0; modelIndex < modelCount; modelIndex++) {
      ModelSaveFormat modelSaveFormat = saveFormat.models[modelIndex];
      Assert(modelSaveFormat.index < modelCount);
      worldModelIndices[modelSaveFormat.index] = addNewModel(world, modelSaveFormat.fileName.c_str());

      Model* model = world->models + worldModelIndices[modelSaveFormat.index];
      for(u32 meshIndex = 0; meshIndex < model->meshCount; meshIndex++) {
        Mesh* mesh = model->meshes + meshIndex;
        mesh->textureData.baseColor = modelSaveFormat.baseColor;
      }
    }
  }

  u32* worldSceneIndices = new u32[sceneCount];
  { // scenes
    for(u32 sceneIndex = 0; sceneIndex < sceneCount; sceneIndex++) {
      SceneSaveFormat sceneSaveFormat = saveFormat.scenes[sceneIndex];
      size_t entityCount = sceneSaveFormat.entities.size();

      Assert(sceneSaveFormat.index < sceneCount);
      worldSceneIndices[sceneSaveFormat.index] = addNewScene(world, sceneSaveFormat.title.c_str());
      Scene* scene = world->scenes + worldSceneIndices[sceneSaveFormat.index];
      scene->title = cStrAllocateAndCopy(sceneSaveFormat.title.c_str());

      if(!sceneSaveFormat.skyboxDir.empty() && !sceneSaveFormat.skyboxExt.empty()) { // if we have a skybox...
        scene->skyboxDir = cStrAllocateAndCopy(sceneSaveFormat.skyboxDir.c_str());
        scene->skyboxExt = cStrAllocateAndCopy(sceneSaveFormat.skyboxExt.c_str());
        loadCubeMapTexture(scene->skyboxDir, scene->skyboxExt, &scene->skyboxTexture);
      } else {
        scene->skyboxTexture = TEXTURE_ID_NO_TEXTURE;
      }

      for(u32 entityIndex = 0; entityIndex < entityCount; entityIndex++) {
        EntitySaveFormat entitySaveFormat = sceneSaveFormat.entities[entityIndex];
        addNewEntity(world, worldSceneIndices[sceneSaveFormat.index], worldModelIndices[entitySaveFormat.modelIndex],
                     entitySaveFormat.posXYZ, entitySaveFormat.scaleXYZ, entitySaveFormat.yaw,
                     worldShaderIndices[entitySaveFormat.shaderIndex], entitySaveFormat.flags);
      }
    }

    // we have to iterate over the scenes once more for portals, as the scene destination index requires
    // the other scenes to have been initialized
    for(u32 sceneIndex = 0; sceneIndex < sceneCount; sceneIndex++)
    {
      SceneSaveFormat sceneSaveFormat = saveFormat.scenes[sceneIndex];
      size_t portalCount = sceneSaveFormat.portals.size();
      Assert(portalCount <= MAX_PORTALS);
      for (u32 portalIndex = 0; portalIndex < portalCount; portalIndex++)
      {
        PortalSaveFormat portalSaveFormat = sceneSaveFormat.portals[portalIndex];
        addPortal(world, worldSceneIndices[sceneSaveFormat.index], portalSaveFormat.centerXYZ, portalSaveFormat.normalXYZ, portalSaveFormat.dimensXY,
                  portalIndex + 1, worldSceneIndices[portalSaveFormat.destination]);
      }
    }
  }

  Assert(saveFormat.startingSceneIndex < sceneCount);
  world->currentSceneIndex = worldSceneIndices[saveFormat.startingSceneIndex];

  // TODO: create limited length arrays instead of using dynamic memory?
  delete[] worldShaderIndices;
  delete[] worldModelIndices;
  delete[] worldSceneIndices;
  return;
}

void portalScene(GLFWwindow* window) {
  vec2_u32 windowExtent = getWindowExtent();
  const vec2_u32 initWindowExtent = windowExtent;
  globalWorld.aspect = f32(windowExtent.width) / windowExtent.height;
  glGenQueries(ArrayCount(portalQueryObjects), portalQueryObjects);

  VertexAtt cubePosVertexAtt = cubePosVertexAttBuffers();

  initGlobalShaders();
  initGlobalVertexAtts();

  globalWorld.player.boundingBox.diagonal = defaultPlayerDimensionInMeters;
  globalWorld.player.boundingBox.min = {-(globalWorld.player.boundingBox.diagonal.x * 0.5f), -12.0f - (globalWorld.player.boundingBox.diagonal.y * 0.5f), 0.0f};

  vec3 firstPersonCameraInitPosition = calcPlayerViewingPosition(&globalWorld.player);
  vec3 firstPersonCameraInitFocus{0, 0, firstPersonCameraInitPosition.z};
  lookAt_FirstPerson(firstPersonCameraInitPosition, firstPersonCameraInitFocus, &globalWorld.camera);

  globalWorld.fov = fieldOfView(13.5f, 25.0f);
  globalWorld.projectionViewModelUbo.projection = perspective(globalWorld.fov, globalWorld.aspect, near, far);

  loadWorld(&globalWorld, originalSceneLoc); // TODO: Choose through IMGUI

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
  GLuint gateSceneFragUBOid;
  LightUBO lightUbo;
  {
    glGenBuffers(1, &projViewModelGlobalUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewModelUBO), NULL, GL_STREAM_DRAW);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, projectionViewModelUBOBindingIndex, projViewModelGlobalUBOid, 0, sizeof(ProjectionViewModelUBO));

    glGenBuffers(1, &fragUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, fragUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FragUBO), NULL, GL_STREAM_DRAW);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, fragUBOBindingIndex, fragUBOid, 0, sizeof(FragUBO));

    // TODO: Specify light in JSON for scene?
    lightUbo.directionalLightColor = {0.5f, 0.5f, 0.5f};
    lightUbo.ambientLightColor = {0.3f, 0.3f, 0.3f};
    lightUbo.directionalLightDirToSource = {1.0f, 1.0f, 1.0f};
    glGenBuffers(1, &gateSceneFragUBOid);
    // allocate size for buffer
    glBindBuffer(GL_UNIFORM_BUFFER, gateSceneFragUBOid);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lightUbo), &lightUbo, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // attach buffer to ubo binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, lightUBOBindingIndex, gateSceneFragUBOid, 0, sizeof(lightUbo));
  }

  globalWorld.stopWatch = createStopWatch();
  bool cursorEnabled = isCursorEnabled(window);
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

    // toggle cursor
    if(hotPress(KeyboardInput_Space)) {
      cursorEnabled = !cursorEnabled;
      enableCursor(window, cursorEnabled);
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
    const bool cameraMovementEnabled = !cursorEnabled;
    if(cameraMovementEnabled) {
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
    }
    globalWorld.projectionViewModelUbo.view = getViewMat(globalWorld.camera);

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
    f32 scaleElapsedTime = globalWorld.stopWatch.totalElapsed * 0.2f;
    lightUbo.directionalLightDirToSource = normalize(vec3{cosf(scaleElapsedTime) * 3.0f, sinf(scaleElapsedTime) * 3.0f, 1.0f});
    glBufferSubData(GL_UNIFORM_BUFFER, offsetof(LightUBO, directionalLightDirToSource), sizeof(vec3), &lightUbo.directionalLightDirToSource);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if(globalWorld.camera.thirdPerson) { // draw player if third person
      vec3 playerViewCenter = calcPlayerViewingPosition(&globalWorld.player);
      vec3 playerBoundingBoxColor_Red{1.0f, 0.0f, 0.0f};
      vec3 playerViewBoxColor_White{1.0f, 1.0f, 1.0f};
      vec3 playerMinCoordBoxColor_Green{0.0f, 1.0f, 0.0f};
      vec3 playerMinCoordBoxColor_Black{0.0f, 0.0f, 0.0f};

      mat4 thirdPersonPlayerBoxesModelMatrix;

      glUseProgram(globalShaders.singleColor.id);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);

      // debug player bounding box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(globalWorld.player.boundingBox.diagonal, playerCenter);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      setUniform(globalShaders.singleColor.id, baseColorUniformName, playerBoundingBoxColor_Red);
      drawTriangles(&cubePosVertexAtt);

      // debug player center
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(0.05f, playerCenter);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      setUniform(globalShaders.singleColor.id, baseColorUniformName, playerMinCoordBoxColor_Black);
      drawTriangles(&cubePosVertexAtt);

      // debug player min coordinate box
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(0.1f, globalWorld.player.boundingBox.min);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      setUniform(globalShaders.singleColor.id, baseColorUniformName, playerMinCoordBoxColor_Green);
      drawTriangles(&cubePosVertexAtt);

      // debug player view
      glBindBuffer(GL_UNIFORM_BUFFER, projViewModelGlobalUBOid);
      thirdPersonPlayerBoxesModelMatrix = scaleTrans_mat4(0.1f, playerViewCenter);
      glBufferSubData(GL_UNIFORM_BUFFER, offsetof(ProjectionViewModelUBO, model), sizeof(mat4), &thirdPersonPlayerBoxesModelMatrix);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      setUniform(globalShaders.singleColor.id, baseColorUniformName, playerViewBoxColor_White);
      drawTriangles(&cubePosVertexAtt);

      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    updateEntities(&globalWorld);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool trueBool = true;

    if (ImGui::BeginMainMenuBar())
    {
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Load..", "Ctrl+O")) {
          // load world
          ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".cpp,.h,.hpp", ".");
        }
        if (ImGui::MenuItem("Save As..", "Ctrl+S"))   {
          // save current world
          saveWorld(&globalWorld, originalSceneLoc);
        }

        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    // display
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
      // action if OK
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
        std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
        // action
      }

      // close
      ImGuiFileDialog::Instance()->Close();
    }

//    ImGui::ShowDemoWindow(&trueBool);

    // Rendering
    ImGui::Render();

    drawSceneWithPortals(&globalWorld);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window); // swaps double buffers (call after all render commands are completed)
    glfwPollEvents(); // checks for events (ex: keyboard/mouse input)
  }

  deleteShaderPrograms(globalShaders.shaders, ArrayCount(globalShaders.shaders));
}