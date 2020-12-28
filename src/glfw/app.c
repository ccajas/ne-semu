#include "app.h"
#include "callbacks.h"
#include "inputstates.h"
#include "../../nfd/src/nfd.h"

void app_init(App * app)
{
    app->window = glfw_new_window (app->resolution[0], app->resolution[1], app->title);

    /* Initialize graphics and emulation system */
    graphics_init (&app->scene);
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

uint8_t app_controller_state (KeyboardState * const key)
{
    uint8_t controller = 0;
	if (input_key (key, GLFW_KEY_K))     controller |= 0x80; /* A */
	if (input_key (key, GLFW_KEY_L))     controller |= 0x40; /* B */
	if (input_key (key, GLFW_KEY_G))     controller |= 0x20; /* Select */
	if (input_key (key, GLFW_KEY_ENTER)) controller |= 0x10; /* Start */
	if (input_key (key, GLFW_KEY_W))     controller |= 0x08;
	if (input_key (key, GLFW_KEY_S))     controller |= 0x04;
	if (input_key (key, GLFW_KEY_A))     controller |= 0x02;
	if (input_key (key, GLFW_KEY_D))     controller |= 0x01;

    return controller;
}

void app_handle_input (App * app)
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

void app_update (App * app)
{
    app_handle_input (app);

    /* Const aliases */
    KeyboardState * key     = &app->keyboardState;
    KeyboardState * lastKey = &app->lastKeyboardState;

    /* Standard app functions */
    if (input_new_key (key, lastKey, GLFW_KEY_F11)) app_toggle_maximize (app);
    if (input_new_key (key, lastKey, GLFW_KEY_O))   app_open_dialog();
    if (input_new_key (key, lastKey, GLFW_KEY_ESCAPE) || 
        glfwWindowShouldClose(app->window)) app->running = 0;

    /* Emulation and debug functions */
    if (input_new_key (key, lastKey, GLFW_KEY_R)) bus_reset (&NES);
    if (input_new_key (key, lastKey, GLFW_KEY_X)) app->emulationRun = ~app->emulationRun;
    if (input_new_key (key, lastKey, GLFW_KEY_Q)) ppu_toggle_debug (&NES.ppu);
    if (input_new_key (key, lastKey, GLFW_KEY_Y)) {
        copy_pattern_table (&NES.ppu, 0);
        copy_pattern_table (&NES.ppu, 1);
    }

    /* Controller buttons */
    NES.controller[0] = app_controller_state (key);

    /* Update emulator in real time or step through cycles */
    if (app->emulationRun) {
        bus_exec (&NES, 29829);
    }
    else {
        if (input_key     (key, GLFW_KEY_C))          { bus_scanline_step (&NES); }
        if (input_new_key (key, lastKey, GLFW_KEY_Z)) { bus_cpu_tick (&NES); }
    }

    /* Update window title */
    char textbuf[64];
    sprintf(textbuf, "Frame time: %f ms", app->timer.frameTime);
    glfwSetWindowTitle(app->window, textbuf);
    glfwPollEvents();
}

void app_draw(App * app)
{
    draw_scene (app->window, &app->scene);
    draw_debug (app->window, &app->timer);

    glfwSwapBuffers(app->window);
}

void app_free(App * app)
{
    glfwDestroyWindow(app->window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

/* Callback wrappers */

void app_toggle_maximize (App* const app)
{
    if (!glfwGetWindowAttrib(app->window, GLFW_MAXIMIZED)) {
        glfwMaximizeWindow (app->window);
    } else {
        glfwRestoreWindow (app->window);
    }   
}

void app_capture_mouse_scroll (App * app, double xOffset, double yOffset)
{
    app->mouseState.scrollX = xOffset;
    app->mouseState.scrollY = yOffset;
}

void app_capture_drop (App * app, char * paths[])
{
    app->dropPath = paths[0];

    /* Attempt to load the file */
    rom_load (&NES, app->dropPath);
}

void app_open_dialog()
{
    char *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog ("nes", "./ne-semu", &outPath);

    if (result == NFD_OKAY) {
        rom_load (&NES, outPath);
    }
}