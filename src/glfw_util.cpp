void toWindowedMode(GLFWwindow* window, const u32 width, const u32 height) {
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  u32 centeringUpperLeftX = (mode->width / 2) - (width / 2);
  u32 centeringUpperLeftY = (mode->height / 2) - (height / 2);
  glfwSetWindowMonitor(window, NULL/*Null for windowed mode*/, centeringUpperLeftX, centeringUpperLeftY, width, height, GLFW_DONT_CARE);
}

vec2_u32 toFullScreenMode(GLFWwindow* window) {
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
  return vec2_u32{(u32)Max(0, mode->width), (u32)Max(0, mode->height) };
}

vec2_u32 toggleWindowSize(GLFWwindow* window, const u32 width, const u32 height)
{
  vec2_u32 resultWindowExtent{width, height };
  func_persist bool windowMode = true;
  if (windowMode) {
    resultWindowExtent = toFullScreenMode(window);
  } else{
    toWindowedMode(window, width, height);
  }
  windowMode = !windowMode;
  return resultWindowExtent;
}