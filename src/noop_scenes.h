#pragma once

#define GLFW_INCLUDE_NONE // ensure GLFW doesn't load OpenGL headers
#include <glfw3.h>
#undef APIENTRY // collides with minwindef.h
#include <glad/glad.h>
#include <iostream>
#include <math.h>
#include <string>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

#include "noop_types.h"
#include "noop_math.h"
#include "vertex_attributes.h"
#include "file_locations.h"
#include "input.h"
#include "shader_program.h"
#include "textures.h"
#include "util.h"
#include "uniform_buffer_object_structs.h"
#include "model.h"
#include "camera.h"

#include "glfw_util.cpp"
#include "input.cpp"
#include "portal_scene.cpp"
#include "vertex_attributes.cpp"
