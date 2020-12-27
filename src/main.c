#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "app.h"

int main (int argc, char** argv)
{
    App app = {
        .dropPath       = NULL,
        .title          = "ne-semu",
        .onScroll       = glfw_cb_scroll,
        .onDrop         = glfw_cb_drop,
        .onWindowResize = glfw_cb_window_size,
        .resolution     = { 1280, 720 }
    };
    app.scene = (Scene){ .bgColor = { 113, 115, 126 } };
    app_init (&app);

    /* Time elapsed and tracking */
    double lastTime  = glfwGetTime();
    double deltaTime = 0, currentTime = 0;
    const double FPS = 60.0f;
    const double limitFPS = 1.0f / FPS;
    uint8_t update = 0;

    /* Main update loop */
    while (app.running)
    {
        currentTime = glfwGetTime();
        deltaTime += (currentTime - lastTime);
        lastTime = currentTime;

        if (deltaTime >= limitFPS)
        {
            app_update (&app);
            deltaTime -= limitFPS;
            update++;
        }

        app_draw (&app);
    }
 
    app_free(&app);
    return 0;
}
 