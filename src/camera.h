#pragma once

#include "noop_math.h"

const Vec3 worldUp = Vec3(0.0f, 0.0f, 1.0f);

struct Camera {
  Vec3 origin;
  Vec3 up;
  Vec3 right;
  Vec3 forward;
  f32 pitch;
  f32 yaw;
};

// NOTE: pitch and yaw are set to radians
// NOTE: There is currently no support for lookAt where forward should point directly up
Camera lookAt(Vec3 origin, Vec3 focus) {
  const f32 forwardDotUpThresholdMax = 0.996194f; // cos(5 degrees)
  const f32 forwardDotUpThresholdMin = -0.996194f; // cos(175 degrees)

  Camera camera;
  camera.origin = origin;
  camera.forward = glm::normalize(focus - origin);
  f32 forwardDotUp = glm::dot(camera.forward, worldUp);
  if (forwardDotUp > forwardDotUpThresholdMax || forwardDotUp < forwardDotUpThresholdMin)
  {
    std::cout << "Look At Camera Failed" << std::endl;
    camera.forward = Vec3(0.0f, 1.0f, 0.0f);
  }

  camera.pitch = glm::asin(camera.forward.z);

  Vec2 cameraForwardXYPlane = glm::normalize(Vec2(camera.forward.x, camera.forward.y));
  f32 cameraFrontDotZeroYaw = glm::dot(cameraForwardXYPlane, Vec2(1.0f, 0.0f));
  camera.yaw = glm::acos(cameraFrontDotZeroYaw);

  camera.right = glm::normalize(glm::cross(camera.forward, worldUp));
  camera.up = glm::cross(camera.right, camera.forward);
  return camera;
}

/*
 * NOTE: Positive pitch offsets follows right hand rule (counter clockwise) with your thumb pointing in direction of X
 * NOTE: Positive yaw offsets follow right hand rule (counter clockwise) with your thumb pointing in direction of Z
 */
void updateCameraFirstPerson(Camera* camera, Vec3 posOffset, f32 pitchOffset, f32 yawOffset) {
  const f32 maxMinPitch = RadiansPerDegree * 85.0f;

  camera->origin += posOffset;

  camera->pitch += pitchOffset;
  if(camera->pitch > maxMinPitch) {
    camera->pitch = maxMinPitch;
  } else if(camera->pitch < -maxMinPitch){
    camera->pitch = -maxMinPitch;
  }

  camera->yaw += yawOffset;
  if(camera->yaw > Tau32) {
    camera->yaw -= Tau32;
  }

  // Calculate the new Front vector
  Vec3 forward;
  f32 cosPitch = cos(camera->pitch);
  forward.x = cos(camera->yaw) * cosPitch;
  forward.y = sin(camera->yaw) * cosPitch;
  forward.z = sin(camera->pitch);

  camera->forward = glm::normalize(forward);
  // Also re-calculate the Right and Up vector
  camera->right = glm::normalize(glm::cross(camera->forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
  camera->up = glm::normalize(glm::cross(camera->right, camera->forward));
  camera->origin = camera->origin;
}

// NOTE: offsetPitch and offsetYaw in radians
Mat4 getViewMatrix(Camera camera) {
  // In glm we access elements as mat[col][row] due to column-major layout
  Mat4 translation = Mat4(
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          -camera.origin.x, -camera.origin.y, -camera.origin.z, 1.0f);

  Mat4 rotation = Mat4(
          camera.right.x, camera.up.x, -camera.forward.x, 0.0f,
          camera.right.y, camera.up.y, -camera.forward.y, 0.0f,
          camera.right.z, camera.up.z, -camera.forward.z, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f);

  // Return lookAt matrix as combination of translation and rotation matrix
  Mat4 resultMat = rotation * translation;
  return resultMat; // Remember to read from right to left (first translation then rotation)
}