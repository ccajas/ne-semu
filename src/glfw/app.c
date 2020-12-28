#include "app.h"
#include "callbacks.h"
#include "inputstates.h"
#include "glfw_input.h"

uint8_t app_controller_state (KeyboardState * const key, struct appInputs * const input)
{
    uint8_t controller = 0;
	if (input_key (key, input->BUTTON_A))      controller |= 0x80; /* A */
	if (input_key (key, input->BUTTON_B))      controller |= 0x40; /* B */
	if (input_key (key, input->BUTTON_SELECT)) controller |= 0x20; /* Select */
	if (input_key (key, input->BUTTON_START))  controller |= 0x10; /* Start */
	if (input_key (key, input->BUTTON_UP))     controller |= 0x08;
	if (input_key (key, input->BUTTON_DOWN))   controller |= 0x04;
	if (input_key (key, input->BUTTON_LEFT))   controller |= 0x02;
	if (input_key (key, input->BUTTON_RIGHT))  controller |= 0x01;

    return controller;
}

void app_update (App * app)
{
    app_query_input (app);

    /* Const aliases */
    KeyboardState * key      = &app->keyboardState;
    KeyboardState * lastKey  = &app->lastKeyboardState;
    struct appInputs * input = &app->inputs;

    /* Standard app functions */
    if (input_new_key (key, lastKey, input->APP_MAXIMIZE))    app_toggle_maximize (app);
    if (input_new_key (key, lastKey, input->APP_OPEN_FILE))   app_open_dialog();
    if (input_new_key (key, lastKey, input->APP_EXIT) || 
        glfwWindowShouldClose(app->window)) app->running = 0;

    /* Emulation and debug functions */
    if (input_new_key (key, lastKey, input->EMULATION_TOGGLE)) app->emulationRun = ~app->emulationRun;
    if (input_new_key (key, lastKey, input->EMULATION_DEBUG))  ppu_toggle_debug (&NES.ppu);
    if (input_new_key (key, lastKey, input->EMULATION_RESET))  bus_reset (&NES);

    /* Controller buttons */
    NES.controller[0] = app_controller_state (key, input);

    /* Update emulator in real time or step through cycles */
    if (app->emulationRun) {
        bus_exec (&NES, 29829);
    }
    else {
        if (input_key     (key,          input->EMULATION_SCANLINE)) { bus_scanline_step (&NES); }
        if (input_new_key (key, lastKey, input->EMULATION_STEP))     { bus_cpu_tick (&NES); }
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
    /*nfdresult_t result = NFD_OpenDialog ("nes", "./ne-semu", &outPath);

    if (result == NFD_OKAY) {
        rom_load (&NES, outPath);
    }*/
}