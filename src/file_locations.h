#pragma once

// ==== shaders ====
#define COMMON_SHADER_BASE "src/shaders/"
const char* posVertexShaderFileLoc = COMMON_SHADER_BASE"Pos.vert";
const char* posNormTexVertexShaderFileLoc = COMMON_SHADER_BASE"PosNormTex.vert";
const char* texVertexShaderFileLoc = COMMON_SHADER_BASE"Tex.vert";
const char* skyboxVertexShaderFileLoc = COMMON_SHADER_BASE"Skybox.vert";
const char* singleColorFragmentShaderFileLoc = COMMON_SHADER_BASE"SingleColor.frag";
const char* skyboxFragmentShaderFileLoc = COMMON_SHADER_BASE"SkyboxYIsUp.frag";
const char* gateFragmentShaderFileLoc = COMMON_SHADER_BASE"Gate.frag";
const char* gateVertexShaderFileLoc = COMMON_SHADER_BASE"Gate.vert";
const char* blackFragmentShaderFileLoc = COMMON_SHADER_BASE"Black.frag";
const char* posNormVertexShaderFileLoc = COMMON_SHADER_BASE"PosNorm.vert";
const char* reflectSkyboxFragmentShaderFileLoc = COMMON_SHADER_BASE"ReflectSkyboxYIsUp.frag";
const char* refractSkyboxFragmentShaderFileLoc = COMMON_SHADER_BASE"RefractSkyboxYIsUp.frag";

// Textures
#define COMMON_TEXTURE_BASE "src/data/textures/"
const char* tiledDisplacement1TextureFileLoc = COMMON_TEXTURE_BASE"tiled_musgrave_texture_1_blur.png";

// Skybox Cube Map textures
#define skybox(folder, extension) {  \
"src/data/skybox/"#folder"/front."#extension,              \
"src/data/skybox/"#folder"/back."#extension,               \
"src/data/skybox/"#folder"/top."#extension,                \
"src/data/skybox/"#folder"/bottom."#extension,             \
"src/data/skybox/"#folder"/right."#extension,              \
"src/data/skybox/"#folder"/left."#extension                \
}
#define SKYBOX_TEXTURE_LOCATION_INDEX_FRONT 0
#define SKYBOX_TEXTURE_LOCATION_INDEX_BACK 1
#define SKYBOX_TEXTURE_LOCATION_INDEX_TOP 2
#define SKYBOX_TEXTURE_LOCATION_INDEX_BOTTOM 3
#define SKYBOX_TEXTURE_LOCATION_INDEX_RIGHT 4
#define SKYBOX_TEXTURE_LOCATION_INDEX_LEFT 5
const char* yellowCloudFaceLocations[6] = skybox(yellow_cloud, jpg);
const char* calmSeaFaceLocations[6] = skybox(calm_sea, jpg);
const char* skyboxInterstellarFaceLocations[6] = skybox(interstellar, png);
const char* skyboxSpaceLightBlueFaceLocations[6] = skybox(space_light_blue, png);
const char* archFaceLocations[6] = skybox(arch, png);
const char* caveFaceLocations[6] = skybox(cave, png);
const char* darkFaceLocations[6] = skybox(dark, png);
const char* tronFaceLocations[6] = skybox(tron, png);
const char* hotFaceLocations[6] = skybox(hot, png);
const char* planetRingsFaceLocations[6] = skybox(planet_rings, png);
const char* redEclipseFaceLocations[6] = skybox(red_eclipse, png);
const char* coronaFaceLocations[6] = skybox(corona, png);
const char* distanceSunsetFaceLocations[6] = skybox(distant_sunset, jpg);
const char* exosystem1FaceLocations[6] = skybox(exosystem, jpg);
const char* exosystem2FaceLocations[6] = skybox(exosystem_2, jpg);
const char* heavenFaceLocations[6] = skybox(heaven, jpg);
const char* pollutedEarthFaceLocations[6] = skybox(polluted_earth, jpg);
const char* afterRainFaceLocations[6] = skybox(after_rain, jpg);
const char* aqua1FaceLocations[6] = skybox(aqua_4, jpg);
const char* aqua2FaceLocations[6] = skybox(aqua_9, jpg);
const char* flameFaceLocations[6] = skybox(flame, jpg);
const char* grouseFaceLocations[6] = skybox(grouse, jpg);

// ==== Models ====
#define COMMON_MODEL_BASE "src/data/models/"
const char* gateModelLoc = COMMON_MODEL_BASE"Gate.glb";
const char* crystalModelLoc = COMMON_MODEL_BASE"Crystal-1.glb";
const char* cubeModelLoc = COMMON_MODEL_BASE"Cube.glb";
const char* icosphere1ModelLoc = COMMON_MODEL_BASE"Icosphere-1.glb";
const char* icosphere2ModelLoc = COMMON_MODEL_BASE"Icosphere-2.glb";
const char* torusTriangleModelLoc = COMMON_MODEL_BASE"Torus-Triangle.glb";
const char* torusPentagonModelLoc = COMMON_MODEL_BASE"Torus-Pentagon.glb";
const char* pyramidModelLoc = COMMON_MODEL_BASE"Pyramid.glb";
const char* tetrahedronModelLoc = COMMON_MODEL_BASE"Tetrahedron.glb";
const char* octahedronModelLoc = COMMON_MODEL_BASE"Octahedron.glb";
const char* dodecahedronModelLoc = COMMON_MODEL_BASE"Dodecahedron.glb";
const char* icosahedronModelLoc = COMMON_MODEL_BASE"Icosahedron.glb";
const char* mrSaturnModelLoc = COMMON_MODEL_BASE"Mr-Saturn.glb";
const char* spike2ModelLoc = COMMON_MODEL_BASE"Spike-2.glb";
const char* paperModelLoc = COMMON_MODEL_BASE"Paper.glb";
const char* portalBackingModelLoc = COMMON_MODEL_BASE"Portal-Backing-Plus-Gate-Insignia.glb";
