#include <iostream>

#define GLFW_INCLUDE_NONE // ensure GLFW doesn't load OpenGL headers
#include <glfw3.h>
#include <glad/glad.h>
#undef APIENTRY

#include "noop_types.h"
#include "input.h"
#include "portal_scene.cpp"

#define VIEWPORT_INIT_WIDTH 1920
#define VIEWPORT_INIT_HEIGHT 1080

int main();
void loadGLFW();
GLFWwindow* createWindow();
void initializeGLAD();

int main()
{
  loadGLFW();
  GLFWwindow* window = createWindow();
  initializeGLAD();
  initializeInput(window);
  portalScene(window);
  glfwTerminate(); // clean up gl resources
  return 0;
}

void loadGLFW()
{
  int loadSucceeded = glfwInit();
  if (loadSucceeded == GLFW_FALSE)
  {
    std::cout << "Failed to load GLFW" << std::endl;
    exit(-1);
  }
}

void initializeGLAD()
{
  // intialize GLAD to help manage function pointers for OpenGL
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    exit(-1);
  }
}

GLFWwindow* createWindow()
{
  // set what kind of window desired
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL version x._
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // OpenGL version _.x
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#if MULTI_CAMPLING_ON
  glfwWindowHint(GLFW_SAMPLES, 4);
#endif

  // create window
  GLFWwindow* window = glfwCreateWindow(VIEWPORT_INIT_WIDTH, // int Width
                                        VIEWPORT_INIT_HEIGHT, // int Height
                                        "LearnOpenGL", // const char* Title
                                        NULL, // GLFWmonitor* Monitor: Specified for which monitor for fullscreen, NULL for windowed mode
                                        NULL); // GLFWwindow* Share: window to share resources with
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    exit(-1);
  }

  glfwMakeContextCurrent(window);
  enableCursor(window, false);

  return window;
}