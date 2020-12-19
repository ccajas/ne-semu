#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "../app/timer.h"
#include "text.h"

typedef struct Camera_struct Camera;

typedef struct Scene_struct
{
    uint8_t bgColor[3];
    Shader  mainShader, skyboxShader;
}
Scene;

inline void graphics_init()
{
    gladLoadGL();
    glfwSwapInterval(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	/* Init text and framebuffer objects */
	text_init();
	//framebuffer_init_ms (1280, 720, 8);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL); // Backdrop depth trick

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

inline void draw_scene (GLFWwindow * window, Scene * scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void draw_debug_cpu (int32_t const x, int32_t const y)
{
    char textbuf[256];
    const float size = 0.5f;

    text_draw_raised ("STATUS", x, y, size);

    sprintf (textbuf, "PC: $%04x", cpu->r.pc);
    text_draw_raised (textbuf, x , y - 24, size);
    sprintf (textbuf, "A: $%02x [%d]", cpu->r.a, cpu->r.a);
    text_draw_raised (textbuf, x , y - 48, size);
    sprintf (textbuf, "X: $%02x [%d]", cpu->r.x, cpu->r.x);
    text_draw_raised (textbuf, x , y - 72, size);
    sprintf (textbuf, "Y: $%02x [%d]", cpu->r.y, cpu->r.y);
    text_draw_raised (textbuf, x , y - 96, size);
    sprintf (textbuf, "Stack P: $%04x", cpu->r.sp);
    text_draw_raised (textbuf, x , y - 120, size);
}

inline void draw_debug_ram (int32_t const x, int32_t const y, int8_t rows, int8_t cols, int16_t const start)
{
    char textbuf[256];
    const float size = 0.5f;
    int16_t addr = start;

    sprintf(textbuf, " ---- ");
    for (int j = 0; j < cols; j++)
    {
        sprintf(textbuf + strlen(textbuf), "%02x ", j);
    }
    text_draw_raised (textbuf, x, y, size);
    for (int i = 1; i <= rows; i++)
    {
        sprintf(textbuf, "$%04x ", (uint16_t)addr);
        for (int j = 0; j < cols; j++)
        {
            sprintf(textbuf + strlen(textbuf), "%02x ", bus_read(&NES, addr++)); 
        }
        text_draw_raised (textbuf, x, y - (i * 16), size);
    }
}

inline void draw_debug_ppu (int32_t const x, int32_t const y)
{
    ppu_debug (&NES.ppu, x, y, 0);
    ppu_debug (&NES.ppu, x, y, 1);
}

void draw_debug (GLFWwindow * window, Timer * timer)
{
    update_timer (timer, glfwGetTime());

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    char textbuf[256];     
    text_begin (width, height);
    const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
    const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model

    /* Debug CPU and RAM */
    sprintf(textbuf, "PC: $%04x %02x %s Ticks: %d, Cycles: %ld", 
        cpu->lastpc, cpu->opcode, cpu->lastop, cpu->clockticks, cpu->clockCount);
    text_draw_white (textbuf, 16.0f, 64.0f, 0.6f);

    /* Vendor and framerate info */
    sprintf(textbuf, "Vendor: %s Renderer: %s", vendor, renderer);
    text_draw_white (textbuf, 16.0f, 40.0f, 0.6f);
    sprintf(textbuf, "Avg. frame time: %f ms", timer->frameTime);
    text_draw_white (textbuf, 16.0f, 16.0f, 0.6f);

    /* CPU registers and storage locations for program/vars */
    draw_debug_cpu(width - 240, height - 24);
    draw_debug_ram(16, height - 24,  16, 16, 0x0);
    draw_debug_ram(16, height - 336, 16, 16, 0x8000);

    /* Graphical PPU debug */
    draw_debug_ppu(width, height);
}

#endif