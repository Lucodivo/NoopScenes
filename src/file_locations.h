#pragma once

// ==== shaders ====
#define COMMON_SHADER_BASE "src/shaders/"
const char* const posVertexShaderFileLoc = COMMON_SHADER_BASE"PosVertexShader.glsl";
const char* const singleColorFragmentShaderFileLoc = COMMON_SHADER_BASE"SingleColorFragmentShader.glsl";
const char* const skyboxVertexShaderFileLoc = COMMON_SHADER_BASE"SkyboxVertexShader.glsl";
const char* const skyboxFragmentShaderFileLoc = COMMON_SHADER_BASE"SkyboxFragmentShader.glsl";

// Skybox Cube Map textures
#define skybox(folder, extension) {  \
"src/data/skybox/"#folder"/right."#extension,              \
"src/data/skybox/"#folder"/left."#extension,               \
"src/data/skybox/"#folder"/top."#extension,                \
"src/data/skybox/"#folder"/bottom."#extension,             \
"src/data/skybox/"#folder"/front."#extension,              \
"src/data/skybox/"#folder"/back."#extension                \
}
const char* const skyboxYellowCloudFaceLocations[6] = skybox(yellow_cloud, jpg);
const char* const skyboxWaterFaceLocations[6] = skybox(water, jpg);
const char* const skyboxInterstellarFaceLocations[6] = skybox(interstellar, png);
const char* const skyboxSpaceLightBlueFaceLocations[6] = skybox(space_light_blue, png);

// ==== Models ====
#define COMMON_MODEL_BASE "src/data/models/"
const char* const portalModelLoc = COMMON_MODEL_BASE"Gate.glb";