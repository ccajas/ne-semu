#ifndef GLFW_INPUT_H
#define GLFW_INPUT_H

#include <GLFW/glfw3.h>
#include "../gl/graphics.h"

void app_init_inputs (App * app)
{
    /* App event inputs */
    app->inputs.EVENT_OPEN_FILE     = GLFW_KEY_O;
    app->inputs.EVENT_MAXIMIZE      = GLFW_KEY_F11;
    app->inputs.EVENT_CHANGE_SCALE  = GLFW_KEY_MINUS;
    app->inputs.EVENT_EXIT          = GLFW_KEY_ESCAPE;
    app->inputs.EMULATION_PAUSE     = GLFW_KEY_X;
    app->inputs.EMULATION_STEP      = GLFW_KEY_Z;
    app->inputs.EMULATION_SCANLINE  = GLFW_KEY_C;
    app->inputs.EMULATION_RESET     = GLFW_KEY_R;

    /* controller buttons */
    app->inputs.BUTTON_A      = GLFW_KEY_K;
    app->inputs.BUTTON_B      = GLFW_KEY_L;
    app->inputs.BUTTON_SELECT = GLFW_KEY_G;
    app->inputs.BUTTON_START  = GLFW_KEY_ENTER;
    app->inputs.BUTTON_UP     = GLFW_KEY_W;
    app->inputs.BUTTON_DOWN   = GLFW_KEY_S;
    app->inputs.BUTTON_LEFT   = GLFW_KEY_A;
    app->inputs.BUTTON_RIGHT  = GLFW_KEY_D;
}

void app_init(App * app)
{
    app->resolution[0] = app->screenScale * 320;
    app->resolution[1] = app->screenScale * 240;

    app->window = glfw_new_window (app->resolution[0], app->resolution[1], app->title, NULL);
#ifdef PPU_DEBUG
    app->debugWindow = glfw_new_window (512, 256, "PPU Viewer", app->window);
#endif

    /* Initialize graphics and emulation system */
    graphics_init (&app->scene);
    app_init_inputs (app);
    bus_reset (&NES);

    glfwSetWindowUserPointer       (app->window,      app);
#ifdef PPU_DEBUG
    glfwSetWindowUserPointer       (app->debugWindow, app);
    glfwSetWindowSizeCallback      (app->window, app->onWindowResize);
#endif

    /* Setup callbacks for main window */
    glfwSetErrorCallback           (glfw_cb_error);
    glfwSetScrollCallback          (app->window, app->onScroll);
    glfwSetDropCallback            (app->window, app->onDrop);
    glfwSetFramebufferSizeCallback (app->window, app->onWindowResize);
    glfwSetWindowSizeCallback      (app->window, app->onWindowResize);

    /* Setup timer */
    app->timer = (Timer){0};
    app->timer.previousTime = glfwGetTime();

    app->running = 1;
    app->paused = 1;
}

void app_query_input (App * app)
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

void app_free (App * const app)
{
    glfwDestroyWindow(app->window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

/* Callback wrappers */

void app_toggle_maximize (App * const app)
{
    if (!glfwGetWindowAttrib(app->window, GLFW_MAXIMIZED)) {
        glfwMaximizeWindow (app->window);
    } else {
        glfwRestoreWindow (app->window);
    }   
}

#endif
