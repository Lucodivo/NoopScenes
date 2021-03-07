#pragma once

const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

struct Camera {
  glm::vec3 origin;
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 forward;
  f32 pitch;
  f32 yaw;
};

// NOTE: pitch and yaw are set to radians
// NOTE: There is currently no support for lookAt where forward should point directly up
Camera lookAt(glm::vec3 origin, glm::vec3 focus) {
  const f32 forwardDotUpThresholdMax = 0.996194f; // cos(5 degrees)
  const f32 forwardDotUpThresholdMin = -0.996194f; // cos(175 degrees)

  Camera camera;
  camera.origin = origin;
  camera.forward = glm::normalize(focus - origin);
  f32 forwardDotUp = glm::dot(camera.forward, worldUp);
  if (forwardDotUp > forwardDotUpThresholdMax || forwardDotUp < forwardDotUpThresholdMin)
  {
    std::cout << "Look At Camera Failed" << std::endl;
    camera.forward = glm::vec3(0.0f, 0.0f, -1.0f);
  }

  camera.pitch = glm::asin(camera.forward.y);

  glm::vec2 cameraForwardXZPlane = glm::normalize(glm::vec2(camera.forward.x, camera.forward.z));
  f32 cameraFrontDotZeroPitch = glm::dot(cameraForwardXZPlane, glm::vec2(0.0f, -1.0f));
  camera.yaw = glm::acos(cameraFrontDotZeroPitch);

  camera.right = glm::normalize(glm::cross(worldUp, camera.forward));
  camera.up = glm::cross(camera.forward, camera.right);
  return camera;
}

void rotateCamera(Camera* camera, f32 pitchOffset, f32 yawOffset) {
  const f32 maxMinPitch = RadiansPerDegree * 85.0f;

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
  glm::vec3 forward;
  f32 cosPitch = cos(camera->pitch);
  forward.x = sin(camera->yaw) * cosPitch;
  forward.y = sin(camera->pitch);
  forward.z = -cos(camera->yaw) * cosPitch;

  camera->forward = glm::normalize(forward);
  // Also re-calculate the Right and Up vector
  camera->right = glm::normalize(glm::cross(camera->forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
  camera->up = glm::normalize(glm::cross(camera->right, camera->forward));
}

// NOTE: offsetPitch and offsetYaw in radians
glm::mat4 getViewMatrix(Camera camera) {
  // In glm we access elements as mat[col][row] due to column-major layout
  glm::mat4 translation = glm::mat4(
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          -camera.origin.x, -camera.origin.y, -camera.origin.z, 1.0f);

  glm::mat4 rotation = glm::mat4(
          camera.right.x, camera.up.x, -camera.forward.x, 0.0f,
          camera.right.y, camera.up.y, -camera.forward.y, 0.0f,
          camera.right.z, camera.up.z, -camera.forward.z, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f);

  // Return lookAt matrix as combination of translation and rotation matrix
  return rotation * translation; // Remember to read from right to left (first translation then rotation)
}