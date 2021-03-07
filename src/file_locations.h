#pragma once

// ==== shaders ====
#define COMMON_BASE "src/shaders/"
const char* const posVertexShaderFileLoc = COMMON_BASE"PosVertexShader.glsl";
const char* const singleColorFragmentShaderFileLoc = COMMON_BASE"SingleColorFragmentShader.glsl";
const char* const skyboxVertexShaderFileLoc = COMMON_BASE"SkyboxVertexShader.glsl";
const char* const skyboxFragmentShaderFileLoc = COMMON_BASE"SkyboxFragmentShader.glsl";

// Skybox Cube Map textures
#define skybox(folder, extension) {  \
"src/data/skybox/"#folder"/right."#extension,              \
"src/data/skybox/"#folder"/left."#extension,               \
"src/data/skybox/"#folder"/top."#extension,                \
"src/data/skybox/"#folder"/bottom."#extension,             \
"src/data/skybox/"#folder"/front."#extension,              \
"src/data/skybox/"#folder"/back."#extension                \
}
const char* const yellowCloudFaceLocations[6] = skybox(yellow_cloud, jpg);