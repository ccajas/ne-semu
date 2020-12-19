#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
//#define USE_HDR

#if defined (USE_HDR)
#include "utils/stb_image.h"
#endif

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
    app_init (&app);

    /* Main update loop */
    while (app.running)
    {
        app_draw (&app);
        app_update (&app);
    }
 
    app_free(&app);
    return 0;
}
 