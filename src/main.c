#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define MIN_APPs

#ifdef MIN_APP

const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

GLFWwindow * window;

void processInput(GLFWwindow * window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void glfw_cb_framebuffer_size (GLFWwindow * window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void app_setup()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, glfw_cb_framebuffer_size);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
}

void app_run()
{
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}

#else
#include <time.h>
#include "glfw/app.h"
#endif

int main (int argc, char** argv)
{
#ifndef MIN_APP
    App app = {
        .dropPath       = NULL,
        .title          = "ne-semu",
        .onScroll       = glfw_cb_scroll,
        .onDrop         = glfw_cb_drop,
        .onWindowResize = glfw_cb_window_size,
        .resolution     = { 768, 720 }
    };
    app.scene = (Scene){ .bgColor = { 113, 115, 126 } };
    app_init (&app);

    /* Time elapsed and tracking */
    double lastTime  = glfwGetTime();
    double deltaTime = 0, currentTime = 0;
    const double FPS = 60.0f;
    const double limitFPS = 1.0f / FPS;

    /* Main update loop */
    while (app.running)
    {
        currentTime = glfwGetTime();
        deltaTime += (currentTime - lastTime);
        lastTime = currentTime;

        if (deltaTime >= limitFPS)
        {
            deltaTime -= limitFPS;
            app_update (&app);
            app_draw (&app);
        }
    }
 
    app_free(&app);
#else
    app_setup();
    app_run();
#endif
    return 0;
}