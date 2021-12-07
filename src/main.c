#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include "app.h"

int main (int argc, char** argv)
{
#ifndef MIN_APP
    App app = {
        .dropPath       = NULL,
        .title          = "ne-semu",
        .onScroll       = glfw_cb_scroll,
        .onDrop         = glfw_cb_drop,
        .onWindowResize = glfw_cb_window_size,
        .screenScale    = 3
    };
    app.scene = (Scene){ .bgColor = { 113, 115, 186 } };
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