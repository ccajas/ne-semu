#include "app.h"
#include "callbacks.h"
#include "inputstates.h"
#include "glfw_input.h"
#include "../../nfd/src/nfd.h"

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
    if (input_new_key (key, lastKey, input->EVENT_MAXIMIZE))    app_toggle_maximize (app);
    if (input_new_key (key, lastKey, input->EVENT_OPEN_FILE))   app_open_dialog (app);
    if (input_new_key (key, lastKey, input->EVENT_EXIT) || 
        glfwWindowShouldClose(app->window)) app->running = 0;

    /* Emulation and debug functions */
    if (input_new_key (key, lastKey, input->EMULATION_PAUSE)) 
    {
        app->paused = !app->paused;
        printf("Emulator %s\n", app->paused ? "paused" : "running");
    }
    if (input_new_key (key, lastKey, input->EMULATION_DEBUG))  ppu_toggle_debug (&NES.ppu);
    if (input_new_key (key, lastKey, input->EMULATION_RESET))  bus_reset (&NES);

    /* Controller buttons */
    NES.controller[0] = app_controller_state (key, input);

    /* Update emulator in real time or step through cycles */
    if (!app->paused) {
        bus_exec (&NES, 29829);
    }
    else {
        if (input_key     (key,          input->EMULATION_SCANLINE)) { bus_scanline_step (&NES); }
        if (input_new_key (key, lastKey, input->EMULATION_STEP))     { bus_cpu_tick (&NES); }
    }

    /* Update window title */
    char textbuf[256];
    snprintf(textbuf, sizeof(textbuf), "Frame time: %.3f ms | %s", app->timer.frameTime, NES.rom.filename);
    update_timer (&app->timer, glfwGetTime());

    glfwSetWindowTitle(app->window, textbuf);
    glfwPollEvents();
}

void app_draw (App * const app)
{
    draw_scene (app->window, &app->scene);
#ifdef PPU_DEBUG
    draw_debug (app->window, &app->timer);
#endif

    glfwSwapBuffers(app->window);
}

/* Callback wrappers */

void app_capture_mouse_scroll (App * const app, double xOffset, double yOffset)
{
    app->mouseState.scrollX = xOffset;
    app->mouseState.scrollY = yOffset;
}

void app_capture_drop (App * app, char * paths[])
{
    app->dropPath = paths[0];

    /* Attempt to load the file */
    if (rom_load (&NES, app->dropPath))
        app->paused = 0;
}

void app_open_dialog(App * const app)
{
    char *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog ("nes", "./ne-semu", &outPath);

    if (result == NFD_OKAY) 
    {
        if (rom_load (&NES, outPath)) 
            app->paused = 0;
    }
}