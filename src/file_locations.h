#pragma once

// ==== shaders ====
#define COMMON_SHADER_BASE "src/shaders/"
const char* posVertexShaderFileLoc = COMMON_SHADER_BASE"PosVertexShader.glsl";
const char* singleColorFragmentShaderFileLoc = COMMON_SHADER_BASE"SingleColorFragmentShader.glsl";
const char* skyboxVertexShaderFileLoc = COMMON_SHADER_BASE"SkyboxVertexShader.glsl";
const char* skyboxFragmentShaderFileLoc = COMMON_SHADER_BASE"SkyboxFragmentShader.glsl";

// Skybox Cube Map textures
#define skybox(folder, extension) {  \
"src/data/skybox/"#folder"/right."#extension,              \
"src/data/skybox/"#folder"/left."#extension,               \
"src/data/skybox/"#folder"/top."#extension,                \
"src/data/skybox/"#folder"/bottom."#extension,             \
"src/data/skybox/"#folder"/front."#extension,              \
"src/data/skybox/"#folder"/back."#extension                \
}
const char* skyboxYellowCloudFaceLocations[6] = skybox(yellow_cloud, jpg);
const char* skyboxWaterFaceLocations[6] = skybox(water, jpg);
const char* skyboxInterstellarFaceLocations[6] = skybox(interstellar, png);
const char* skyboxSpaceLightBlueFaceLocations[6] = skybox(space_light_blue, png);

// ==== Models ====
#define COMMON_MODEL_BASE "src/data/models/"
const char* gateModelLoc = COMMON_MODEL_BASE"Gate.glb";
const char* crystalModelLoc = COMMON_MODEL_BASE"Crystal-1.glb";
const char* cubeModelLoc = COMMON_MODEL_BASE"Cube.glb";
const char* icosphere1ModelLoc = COMMON_MODEL_BASE"Icosphere-1.glb";
const char* torusTriangleModelLoc = COMMON_MODEL_BASE"Torus-Triangle.glb";
const char* pyramidModelLoc = COMMON_MODEL_BASE"Pyramid.glb";