#include <stdio.h>
#include <stdlib.h>
#include "callbacks.h"

void glfw_cb_error (int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void glfw_cb_scroll (GLFWwindow * window, double xoffset, double yoffset)
{
    App* app = glfwGetWindowUserPointer (window);
    app_capture_mouse_scroll (app, xoffset, yoffset);
}

void glfw_cb_drop (GLFWwindow * window, int count, char * paths[])
{
    App* app = glfwGetWindowUserPointer (window);
    app_capture_drop (app, paths);
}

void glfw_cb_window_size (GLFWwindow * window, int width, int height)
{
    if (height % 240 == 0) return;

    /*uint16_t nHeight = (height / 240) * 240;
    if (nHeight < 240) nHeight = 240;
    uint16_t nWidth  = (nHeight / 15) * 16;

    glfwSetWindowSize (window, nWidth, nHeight);
    glViewport (0, 0, nWidth, nHeight); */
}

GLFWwindow * glfw_new_window(int width, int height, const char *title, GLFWwindow * shared)
{
    GLFWwindow* window;

    if (!glfwInit())
        exit(EXIT_FAILURE);
 
    /* Hints for GL configuration */
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_SAMPLES, 4);
    glfwWindowHint (GLFW_RESIZABLE, GL_TRUE);
    /*glfwWindowHint( GLFW_DOUBLEBUFFER, GL_FALSE);*/
 
    window = glfwCreateWindow(width, height, title, NULL, shared);
    if (!window)
    {
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

    return window;
}