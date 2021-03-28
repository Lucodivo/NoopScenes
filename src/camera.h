#pragma once

#include "noop_3d_math.h"

const Vec3 WORLD_UP = Vec3(0.0f, 0.0f, 1.0f);

const f32 MAX_MIN_PITCH_FIRST_PERSON= RadiansPerDegree * 85.0f;
const f32 MIN_PITCH_THIRD_PERSON = -25.0f * RadiansPerDegree;
const f32 MAX_PITCH_THIRD_PERSON = 65.0f * RadiansPerDegree;
const f32 DIST_FROM_PIVOT_THIRD_PERSON = 8.0f;


struct Camera {
  Vec3 origin;
  Vec3 up;
  Vec3 right;
  Vec3 forward;
  f32 pitch;
  f32 yaw;
  b32 thirdPerson;
};

// NOTE: pitch and yaw are set to radians
// NOTE: There is currently no support for lookAt_FirstPerson where forward should point directly up
Camera lookAt_FirstPerson(Vec3 origin, Vec3 focus) {
  const f32 forwardDotUpThresholdMax = 0.996194f; // cos(5 degrees)
  const f32 forwardDotUpThresholdMin = -0.996194f; // cos(175 degrees)

  Camera camera;
  camera.thirdPerson = false;
  camera.origin = origin;
  camera.forward = glm::normalize(focus - origin);
  f32 forwardDotUp = glm::dot(camera.forward, WORLD_UP);
  if (forwardDotUp > forwardDotUpThresholdMax || forwardDotUp < forwardDotUpThresholdMin)
  {
    std::cout << "Look At Camera Failed" << std::endl;
    camera.forward = glm::normalize(Vec3(camera.forward.x, camera.forward.y + 0.01f, 0.0f));
  }

  camera.pitch = glm::asin(camera.forward.z);

  Vec2 cameraForwardXYPlane = glm::normalize(Vec2(camera.forward.x, camera.forward.y));
  camera.yaw = glm::acos(cameraForwardXYPlane.x);
  if(cameraForwardXYPlane.y < 0) {
    camera.yaw = -camera.yaw;
  }

  camera.right = glm::normalize(glm::cross(camera.forward, WORLD_UP));
  camera.up = glm::cross(camera.right, camera.forward);
  return camera;
}

/*
 * NOTE: Positive pitch offsets follows right hand rule (counter clockwise) with your thumb pointing in direction of X
 * NOTE: Positive yaw offsets follow right hand rule (counter clockwise) with your thumb pointing in direction of Z
 */
void updateCamera_FirstPerson(Camera* camera, Vec3 posOffset, f32 pitchOffset, f32 yawOffset) {
  Assert(!camera->thirdPerson);
  camera->origin += posOffset;

  camera->pitch += pitchOffset;
  if(camera->pitch > MAX_MIN_PITCH_FIRST_PERSON) {
    camera->pitch = MAX_MIN_PITCH_FIRST_PERSON;
  } else if(camera->pitch < -MAX_MIN_PITCH_FIRST_PERSON){
    camera->pitch = -MAX_MIN_PITCH_FIRST_PERSON;
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
  camera->right = glm::normalize(glm::cross(camera->forward, WORLD_UP));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
  camera->up = glm::normalize(glm::cross(camera->right, camera->forward));
  camera->origin = camera->origin;
}

// NOTE: Yaw value of 0 degrees means we are looking in the direction of +x, 90=+y, 180=-x, 270=-y
// NOTE: Pitch value represents the angle between the vector from camera to pivot and the xy plane
Camera lookAt_ThirdPerson(Vec3 pivot, Vec3 forward) {
  // Viewing angle measured between vector from pivot to camera and the xy plane
  const f32 startingPitch = 33.0f * RadiansPerDegree;

  Vec2 xyForward = glm::normalize(Vec2(forward.x, forward.y));
  Vec2 xyPivotToCamera = -xyForward;

  Camera camera;
  camera.thirdPerson = true;
  camera.pitch = startingPitch;
  camera.yaw = acos(xyPivotToCamera.x);
  if(xyPivotToCamera.y < 0) {
    camera.yaw = -camera.yaw;
  }

  const f32 distBackFromPivot = DIST_FROM_PIVOT_THIRD_PERSON * cos(camera.pitch);
  const f32 distAboveFromPivot = DIST_FROM_PIVOT_THIRD_PERSON * sin(camera.pitch);

  camera.origin = pivot + Vec3(-distBackFromPivot * xyForward, distAboveFromPivot);
  camera.forward = glm::normalize(pivot - camera.origin);

  camera.right = glm::normalize(glm::cross(camera.forward, WORLD_UP));
  camera.up = glm::cross(camera.right, camera.forward);
  return camera;
}

void updateCamera_ThirdPerson(Camera* camera, Vec3 pivotPoint, f32 pitchOffset, f32 yawOffset) {
  Assert(camera->thirdPerson);

  camera->pitch += pitchOffset;
  if(camera->pitch > MAX_PITCH_THIRD_PERSON) {
    camera->pitch = MAX_PITCH_THIRD_PERSON;
  } else if(camera->pitch < MIN_PITCH_THIRD_PERSON){
    camera->pitch = MIN_PITCH_THIRD_PERSON;
  }

  camera->yaw += yawOffset;
  if(camera->yaw > Tau32) {
    camera->yaw -= Tau32;
  }

  const f32 distXYFromPivot = DIST_FROM_PIVOT_THIRD_PERSON * cos(camera->pitch);
  const f32 distAboveFromPivot = DIST_FROM_PIVOT_THIRD_PERSON * sin(camera->pitch);
  const f32 distXFromPivot = distXYFromPivot * cos(camera->yaw);
  const f32 distYFromPivot = distXYFromPivot * sin(camera->yaw);

  camera->origin = pivotPoint + Vec3(distXFromPivot, distYFromPivot, distAboveFromPivot);
  camera->forward = glm::normalize(pivotPoint - camera->origin);

  camera->right = glm::normalize(glm::cross(camera->forward, WORLD_UP));
  camera->up = glm::cross(camera->right, camera->forward);
}

// NOTE: offsetPitch and offsetYaw in radians
Mat4 getViewMatrix(Camera camera) {
  // In glm we access elements as mat[col][row] due to column-major layout
  Mat4 translation = Mat4(
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          -camera.origin.x, -camera.origin.y, -camera.origin.z, 1.0f);

  // The camera matrix "measures" the world against it's axes
  // OpenGL clips down the negative z-axis so we negate our forward to effectively cancel out that negation
  Mat4 measureRotation = Mat4(
          camera.right.x, camera.up.x, -camera.forward.x, 0.0f,
          camera.right.y, camera.up.y, -camera.forward.y, 0.0f,
          camera.right.z, camera.up.z, -camera.forward.z, 0.0f,
          0.0f,           0.0f,         0.0f,             1.0f);

  // Return lookAt_FirstPerson matrix as combination of translation and measureRotation matrix
  // Since matrices should be read right to left we want to...
  //    - First center the camera at the origin by translating itself and the entire world
  //    - Then we measure how the world lines up with the camera at the origin
  // All rotation vectors in this instance are orthonormal, so no stretching/squashing occurs is present in the
  // resulting matrix. All angles and area preserved.
  Mat4 resultMat = measureRotation * translation;
  return resultMat; // Remember to read from right to left (first translation then measureRotation)
}