#pragma once
#include <glm/glm.hpp>

/*
 * Insulates glm types from being stamped into the project
 * for easier replacement when desired
 * */
#define Vec2 glm::vec2
#define Vec3 glm::vec3
#define Vec4 glm::vec4
#define Mat3 glm::mat3
#define Mat4 glm::mat4

const Mat4 mat4_identity = Mat4(1.0f);