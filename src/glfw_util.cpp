#define max(a, b) (a > b) ? a : b

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
  return Extent2D{ u32(max(0, mode->width)), u32(max(0, mode->height)) };
}