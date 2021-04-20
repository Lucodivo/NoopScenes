void toWindowedMode(GLFWwindow* window, const u32 width, const u32 height) {
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  u32 centeringUpperLeftX = (mode->width / 2) - (width / 2);
  u32 centeringUpperLeftY = (mode->height / 2) - (height / 2);
  glfwSetWindowMonitor(window, NULL/*Null for windowed mode*/, centeringUpperLeftX, centeringUpperLeftY, width, height, GLFW_DONT_CARE);
}

Extent2D toFullScreenMode(GLFWwindow* window) {
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
  return Extent2D{ (u32)Max(0, mode->width), (u32)Max(0, mode->height) };
}

Extent2D toggleWindowSize(GLFWwindow* window, const u32 width, const u32 height)
{
  Extent2D resultWindowExtent{ width, height };
  local_access bool windowMode = true;
  if (windowMode) {
    resultWindowExtent = toFullScreenMode(window);
  } else{
    toWindowedMode(window, width, height);
  }
  windowMode = !windowMode;
  return resultWindowExtent;
}