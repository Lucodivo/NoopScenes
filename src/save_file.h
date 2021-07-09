#pragma once

const char* saveFileExt = ".json";

struct EntitySaveFormat {
  u32 modelIndex;
  u32 shaderIndex;
  vec3 posXYZ;
  vec3 scaleXYZ;
  f32 yaw;
  b32 flags;
};

struct PortalSaveFormat {
  u32 destination;
  vec3 normalXYZ;
  vec3 centerXYZ;
  vec2 dimensXY;
};

struct ModelSaveFormat {
  u32 index;
  std::string fileName; // NOTE: currently assumed to be in "src/data/models/"
  vec4 baseColor; // optional, alpha 0 means no value
};

struct ShaderSaveFormat {
  u32 index;
  std::string vertexName;
  std::string fragmentName;
  std::string noiseTextureName; // optional, empty string means no value
};

struct SceneSaveFormat {
  u32 index;
  std::string title;
  std::string skyboxDir; // optional, empty string means no value
  std::string skyboxExt; // optional, empty string means no value
  std::vector<EntitySaveFormat> entities;
  std::vector<PortalSaveFormat> portals;
};

struct SaveFormat {
  u32 startingSceneIndex;
  std::vector<SceneSaveFormat> scenes;
  std::vector<ModelSaveFormat> models;
  std::vector<ShaderSaveFormat> shaders;
};

void save(const SaveFormat& saveFormat, const char* saveFileName) {
  nlohmann::json saveJson{};

  saveJson["startingSceneIndex"] = saveFormat.startingSceneIndex;

  size_t sceneCount = saveFormat.scenes.size();
  size_t modelCount = saveFormat.models.size();
  size_t shaderCount = saveFormat.shaders.size();

  for(size_t modelIndex = 0; modelIndex < modelCount; modelIndex++) {
    ModelSaveFormat modelSaveFormat = saveFormat.models[modelIndex];
    saveJson["models"].push_back({
      {"index", modelSaveFormat.index},
      {"fileName", modelSaveFormat.fileName}
    });
    if(modelSaveFormat.baseColor.a != 0.0f) {
      saveJson["models"][modelIndex]["baseColor"] = {
              modelSaveFormat.baseColor.r,
              modelSaveFormat.baseColor.g,
              modelSaveFormat.baseColor.b,
              modelSaveFormat.baseColor.a
      };
    }
  }

  for(u32 shaderIndex = 0; shaderIndex < shaderCount; shaderIndex++) {
    ShaderSaveFormat shaderSaveFormat = saveFormat.shaders[shaderIndex];
    saveJson["shaders"].push_back({
      {"index", shaderSaveFormat.index},
      {"vertexName", shaderSaveFormat.vertexName},
      {"fragmentName", shaderSaveFormat.fragmentName}
    });
    if(!shaderSaveFormat.noiseTextureName.empty()) {
      saveJson["shaders"][shaderIndex]["noiseTextureName"] = shaderSaveFormat.noiseTextureName;
    }
  }

  for(u32 sceneIndex = 0; sceneIndex < sceneCount; sceneIndex++) {
    SceneSaveFormat sceneSaveFormat = saveFormat.scenes[sceneIndex];
    saveJson["scenes"].push_back({
      {"index", sceneSaveFormat.index}
    });
    if(!sceneSaveFormat.title.empty()) {
      saveJson["scenes"][sceneIndex]["title"] = sceneSaveFormat.title;
    }
    if(!sceneSaveFormat.skyboxDir.empty() && !sceneSaveFormat.skyboxExt.empty()) {
      saveJson["scenes"][sceneIndex]["skyboxDir"] = sceneSaveFormat.skyboxDir;
      saveJson["scenes"][sceneIndex]["skyboxExt"] = sceneSaveFormat.skyboxExt;
    }

    size_t entityCount = sceneSaveFormat.entities.size();
    for(size_t entityIndex = 0; entityIndex < entityCount; entityIndex++) {
      EntitySaveFormat entitySaveFormat = sceneSaveFormat.entities[entityIndex];
      saveJson["scenes"][sceneIndex]["entities"].push_back({
        {"modelIndex", entitySaveFormat.modelIndex},
        {"shaderIndex", entitySaveFormat.shaderIndex},
        {"posXYZ", {entitySaveFormat.posXYZ.x, entitySaveFormat.posXYZ.y, entitySaveFormat.posXYZ.z}},
        {"scaleXYZ", {entitySaveFormat.scaleXYZ.x, entitySaveFormat.scaleXYZ.y, entitySaveFormat.scaleXYZ.z}},
        {"yaw", entitySaveFormat.yaw},
        {"flags", entitySaveFormat.flags}
      });
    }

    size_t portalCount = sceneSaveFormat.portals.size();
    for(size_t portalIndex = 0; portalIndex < portalCount; portalIndex++) {
      PortalSaveFormat portalSaveFormat = sceneSaveFormat.portals[portalIndex];
      saveJson["scenes"][sceneIndex]["portals"].push_back({
        {"destination", portalSaveFormat.destination},
        {"centerXYZ", {portalSaveFormat.centerXYZ.x, portalSaveFormat.centerXYZ.y, portalSaveFormat.centerXYZ.z}},
        {"normalXYZ", {portalSaveFormat.normalXYZ.x, portalSaveFormat.normalXYZ.y, portalSaveFormat.normalXYZ.z}},
        {"dimensXY", {portalSaveFormat.dimensXY.x, portalSaveFormat.dimensXY.y}}
      });
    }
  }

  // write prettified JSON to another file
  std::ofstream o(saveFileName);
  o << std::setw(4) << saveJson << std::endl;
}

SaveFormat loadSave(const char* saveJson) {
  SaveFormat saveFormat{};

  // TODO: Should I separate pulling data out from and verifying the json from using the data in the json? (probably!)
  nlohmann::json json;
  { // parse file
    const char* originalSceneLocation = "src/scenes/original_scene.json";
    std::ifstream sceneJsonFileInput(originalSceneLocation);
    sceneJsonFileInput >> json;
  }

  size_t sceneCount = json["scenes"].size();
  size_t modelCount = json["models"].size();
  size_t shaderCount = json["shaders"].size();

  saveFormat.startingSceneIndex = json["startingSceneIndex"];
  Assert(saveFormat.startingSceneIndex < sceneCount);

  saveFormat.scenes.reserve(sceneCount);
  saveFormat.models.reserve(modelCount);
  saveFormat.shaders.reserve(shaderCount);

  { // shaders
    for(u32 jsonShaderIndex = 0; jsonShaderIndex < shaderCount; jsonShaderIndex++) {
      nlohmann::json shaderJson = json["shaders"][jsonShaderIndex];
      ShaderSaveFormat shaderSaveFormat;
      shaderSaveFormat.index = shaderJson["index"];
      shaderJson["vertexName"].get_to(shaderSaveFormat.vertexName);
      shaderJson["fragmentName"].get_to(shaderSaveFormat.fragmentName);

      if(!shaderJson["noiseTextureName"].is_null()) {
        shaderJson["noiseTextureName"].get_to(shaderSaveFormat.noiseTextureName);
      }

      saveFormat.shaders.push_back(shaderSaveFormat);
    }
  }

  { // models
    for(u32 jsonModelIndex = 0; jsonModelIndex < modelCount; jsonModelIndex++) {
      nlohmann::json modelJson = json["models"][jsonModelIndex];
      ModelSaveFormat modelSaveFormat;
      modelSaveFormat.index = modelJson["index"];
      modelJson["fileName"].get_to(modelSaveFormat.fileName);

      if(!modelJson["baseColor"].is_null()) {
        Assert(modelJson["baseColor"].size() == 4);
        modelSaveFormat.baseColor = {
                  modelJson["baseColor"][0],
                  modelJson["baseColor"][1],
                  modelJson["baseColor"][2],
                  modelJson["baseColor"][3]
        };
      } else {
        modelSaveFormat.baseColor = {0.0f, 0.0f, 0.0f, 0.0f};
      }

      saveFormat.models.push_back(modelSaveFormat);
    }
  }

  { // scenes
    for(u32 jsonSceneIndex = 0; jsonSceneIndex < sceneCount; jsonSceneIndex++) {
      nlohmann::json sceneJson = json["scenes"][jsonSceneIndex];
      SceneSaveFormat sceneSaveFormat;
      sceneSaveFormat.index = sceneJson["index"];
      Assert(sceneSaveFormat.index < sceneCount);

      if(!sceneJson["title"].is_null()) {
        sceneJson["title"].get_to(sceneSaveFormat.title);
      } else {
        sceneSaveFormat.title.clear();
      }

      if(!sceneJson["skyboxDir"].is_null() && !sceneJson["skyboxExt"].is_null()) { // if we have a skybox...
        sceneJson["skyboxDir"].get_to(sceneSaveFormat.skyboxDir);
        sceneJson["skyboxExt"].get_to(sceneSaveFormat.skyboxExt);
      } else {
        sceneSaveFormat.skyboxDir.clear();
        sceneSaveFormat.skyboxExt.clear();
      }


      size_t entityCount = sceneJson["entities"].size();
      for(u32 jsonEntityIndex = 0; jsonEntityIndex < entityCount; jsonEntityIndex++) {
        nlohmann::json entityJson = sceneJson["entities"][jsonEntityIndex];
        EntitySaveFormat entitySaveFormat;
        entitySaveFormat.modelIndex = entityJson["modelIndex"];
        entitySaveFormat.shaderIndex = entityJson["shaderIndex"];
        Assert(entitySaveFormat.shaderIndex < shaderCount);
        Assert(entityJson["posXYZ"].size() == 3);
        Assert(entityJson["scaleXYZ"].size() == 3);
        entitySaveFormat.posXYZ = {
                entityJson["posXYZ"][0],
                entityJson["posXYZ"][1],
                entityJson["posXYZ"][2]
        };
        entitySaveFormat.scaleXYZ = {
                entityJson["scaleXYZ"][0],
                entityJson["scaleXYZ"][1],
                entityJson["scaleXYZ"][2]
        };
        entitySaveFormat.yaw = entityJson["yaw"];
        entitySaveFormat.flags = entityJson["flags"];
        sceneSaveFormat.entities.push_back(entitySaveFormat);
      }

      size_t portalCount = sceneJson["portals"].size();
      for (u32 jsonPortalIndex = 0; jsonPortalIndex < portalCount; jsonPortalIndex++)
      {
        nlohmann::json portalJson = sceneJson["portals"][jsonPortalIndex];
        PortalSaveFormat portalSaveFormat;
        portalSaveFormat.destination = portalJson["destination"];
        Assert(portalJson["normalXYZ"].size() == 3);
        Assert(portalJson["centerXYZ"].size() == 3);
        Assert(portalJson["dimensXY"].size() == 2);
        portalSaveFormat.normalXYZ = {
                portalJson["normalXYZ"][0],
                portalJson["normalXYZ"][1],
                portalJson["normalXYZ"][2]
        };
        portalSaveFormat.centerXYZ = {
                portalJson["centerXYZ"][0],
                portalJson["centerXYZ"][1],
                portalJson["centerXYZ"][2]
        };
        portalSaveFormat.dimensXY = {
                portalJson["dimensXY"][0],
                portalJson["dimensXY"][1]
        };
        sceneSaveFormat.portals.push_back(portalSaveFormat);
      }

      saveFormat.scenes.push_back(sceneSaveFormat);
    }

    // we have to iterate over the scenes once more for portals, as the scene destination index requires
    // the other scenes to have been initialized
    SceneSaveFormat* sceneSaveFormats = saveFormat.scenes.data();
    for(u32 sceneIndex = 0; sceneIndex < sceneCount; sceneIndex++)
    {
      nlohmann::json sceneJson = json["scenes"][sceneIndex];
      SceneSaveFormat* sceneSaveFormat = sceneSaveFormats + sceneIndex;
    }
  }

  return saveFormat;
}