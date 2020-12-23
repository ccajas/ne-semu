#ifndef APP_H
#define APP_H

#include "bus.h"
#include "gfx/graphics.h"
#include "app/timer.h"
#include "app/glfw_callbacks.h"
#include "app/inputstates.h"
#include "../nfd/src/nfd.h"

typedef void (*appEventPtr)();

typedef struct App_struct
{
    /* Default event handlers */
    appEventPtr onScroll, onDrop, onWindowResize;

    /* Handles */
    GLFWwindow * window;
    char       * dropPath; 
    const char * title;

    /* Resolution (X * Y) */
    uint16_t resolution[2];

    /* Internal state */
    MouseState    mouseState,    lastMouseState;
    KeyboardState keyboardState, lastKeyboardState;
    uint16_t running;
    uint8_t  emulationRun;
    uint8_t  ppuDebug;

    /* App assets */
    Scene scene;
    Timer timer;
}
App;

inline void app_capture_mouse_scroll (App * app, double xOffset, double yOffset)
{
    app->mouseState.scrollX = xOffset;
    app->mouseState.scrollY = yOffset;
}

inline void app_capture_drop (App * app, char * paths[])
{
    app->dropPath = paths[0];

    /* Attempt to load the file */
    rom_load (&NES, app->dropPath);
    free (app->dropPath);
    bus_reset (&NES);
}

inline void app_toggle_maximize (App* const app)
{
    if (!glfwGetWindowAttrib(app->window, GLFW_MAXIMIZED)) {
        glfwMaximizeWindow (app->window);
    } else {
        glfwRestoreWindow (app->window);
    }   
}

inline void app_open_dialog()
{
    char *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog ("nes", "./ne-semu", &outPath);

    if (result == NFD_OKAY) 
    {
        rom_load (&NES, outPath);
        free (outPath);
        bus_reset (&NES);
    }
}

void app_init(App * app)
{
    app->window = glfw_setup_window (app->resolution[0], app->resolution[1], app->title);

    /* Initialize graphics and emulation system */
    graphics_init();
    bus_reset (&NES);

    glfwSetWindowUserPointer       (app->window, app);

    /* Setup main callbacks */
    glfwSetErrorCallback           (glfw_cb_error);
    glfwSetScrollCallback          (app->window, app->onScroll);
    glfwSetDropCallback            (app->window, app->onDrop);
    glfwSetFramebufferSizeCallback (app->window, app->onWindowResize);
    glfwSetWindowSizeCallback      (app->window, app->onWindowResize);

    /* Setup timer */
    app->timer = (Timer){0};
    app->timer.previousTime = glfwGetTime();

    app->running = 1;
    app->emulationRun = 0;
}

inline void app_update_input (App * app)
{
    app->lastMouseState.x = app->mouseState.x;
    app->lastMouseState.y = app->mouseState.y;

    app->lastMouseState.scrollX = app->mouseState.scrollX;
    app->lastMouseState.scrollY = app->mouseState.scrollY;
    app->mouseState.scrollY = 0;

    app->lastMouseState.buttonMask = app->mouseState.buttonMask;
    app->mouseState.buttonMask = 0;

    for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
	{
		app->mouseState.buttonMask |= glfwGetMouseButton(app->window, i) << i;
	}

    for (int j = 0; j < MAX_KEYS; j++)
    {
        app->lastKeyboardState.keys[j] = app->keyboardState.keys[j];
        app->keyboardState.keys[j] = GLFW_RELEASE;
    }

    app->keyboardState.keysPressed = 0;

    for (int i = 32; i < GLFW_KEY_LAST; i++)
    {
        if (glfwGetKey(app->window, i) == GLFW_PRESS)
        {
            for (int j = 0; j < MAX_KEYS; j++)
            {
                if (app->keyboardState.keys[j] == GLFW_RELEASE) {
                    app->keyboardState.keys[j] = i;
                    break;
                }
            }
        }
    }

    double xPos, yPos = -1;
    glfwGetCursorPos (app->window, &xPos, &yPos);

    app->mouseState.x = (int)xPos;
    app->mouseState.y = (int)yPos;
}

inline void app_update (App * app)
{
    app_update_input (app);

    /* Const aliases */
    KeyboardState * key     = &app->keyboardState;
    KeyboardState * lastKey = &app->lastKeyboardState;

    /* Standard app functions */
    if (input_new_key (key, lastKey, GLFW_KEY_F11)) app_toggle_maximize (app);
    if (input_new_key (key, lastKey, GLFW_KEY_O)) app_open_dialog();
    if (input_new_key (key, lastKey, GLFW_KEY_ESCAPE) || 
        glfwWindowShouldClose(app->window)) app->running = 0;

    /* CPU emulation functions */
    if (input_new_key (key, lastKey, GLFW_KEY_R)) bus_reset (&NES);
    if (input_new_key (key, lastKey, GLFW_KEY_X)) app->emulationRun = ~app->emulationRun;
    if (input_new_key (key, lastKey, GLFW_KEY_Q)) ppu_toggle_debug (&NES.ppu);

    /* Check for closing window */
    if (input_new_key (key, lastKey, GLFW_KEY_T)) 
    {
        NES.ppu.palette++;
        printf("Pal colors: %02x %02x %02x %02x\n", 
            ppu_read(&NES.ppu, 0x3f00 + (NES.ppu.palette << 2)),
            ppu_read(&NES.ppu, 0x3f00 + (NES.ppu.palette << 2) + 1),
            ppu_read(&NES.ppu, 0x3f00 + (NES.ppu.palette << 2) + 2),
            ppu_read(&NES.ppu, 0x3f00 + (NES.ppu.palette << 2) + 3));
    }

    /* Update emulator in real time or step through cycles */
    if (app->emulationRun) {
        bus_exec (&NES, 23863);
    }
    else {
        if (input_new_key (key, lastKey, GLFW_KEY_A)) { bus_clock (&NES); }
    }

    /* Update window title */
    char textbuf[64];
    sprintf(textbuf, "Frame time: %f ms", app->timer.frameTime);
    glfwSetWindowTitle(app->window, textbuf);

    glfwSwapBuffers(app->window);
    glfwPollEvents();
}

void app_draw(App * app)
{
    draw_scene (app->window, &app->scene);
    draw_debug (app->window, &app->timer);
}

void app_free(App * app)
{
    glfwDestroyWindow(app->window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

#endif