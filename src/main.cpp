#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>


void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

int main() {

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
  {
    exit(EXIT_FAILURE);
  }

  GLFWwindow* window = glfwCreateWindow(640, 480, "RetroPixel", NULL, NULL);

  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  gladLoadGL(glfwGetProcAddress);


  while (!glfwWindowShouldClose(window))
  {

  }


  glfwDestroyWindow(window);

  glfwTerminate();

  exit(EXIT_SUCCESS);
}


