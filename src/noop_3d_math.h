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

struct BoundingBox {
  Vec3 min;
  Vec3 dimensionInMeters;
};

void adjustNearFarProjection(Mat4* projectionMatrix, f32 zNear, f32 zFar) {
  // Note: logic pulled straight from glm::perspective -> perspectiveRH_NO
  (*projectionMatrix)[2][2] = - (zFar + zNear) / (zFar - zNear);
  (*projectionMatrix)[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
}

inline f32 dot(Vec3 xyz1, Vec3 xyz2) {
  return (xyz1.x * xyz2.x) + (xyz1.y * xyz2.y) + (xyz1.z * xyz2.z);
}

inline f32 distSquared(Vec3 xyz) {
  return dot(xyz, xyz);
}