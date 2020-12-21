#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "../app/timer.h"
#include "text.h"

typedef struct Camera_struct Camera;

typedef struct Scene_struct
{
    uint8_t bgColor[3];
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

    /* Render the PPU framebuffer here */
}

inline void draw_debug_cpu (int32_t const x, int32_t const y)
{
    char textbuf[256];
    const float size = 0.45f;

    text_draw_raised ("STATUS", x, y, size);

    sprintf (textbuf, "PC: $%04x A:$%02x [%03d] X:$%02x [%03d] Y:$%02x [%03d], SP:$%04x", cpu->r.pc, 
        cpu->r.a, cpu->r.a, cpu->r.x, cpu->r.x, cpu->r.y, cpu->r.y, cpu->r.sp);
    text_draw_raised (textbuf, x , y - 16, size);
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

void draw_debug_ppu (int32_t const width, int32_t const height)
{
	/* Test draw, use PPU shader */
    glUseProgram(NES.ppu.fbufferShader.program);
    mat4x4 p;
    mat4x4_ortho(p, 0, width, 0, height, 0, 0.1f);
    glUniformMatrix4fv (glGetUniformLocation(NES.ppu.fbufferShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) p);

    ppu_debug (&NES.ppu, width, height);
}

void draw_debug (GLFWwindow * window, Timer * timer)
{
    update_timer (timer, glfwGetTime());

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    /* Graphical PPU debug */
    draw_debug_ppu(width, height);

    /* Setup text */
    char textbuf[256];     
    text_begin (width, height);
    const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
    const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model

    /* Vendor and framerate info */
    sprintf(textbuf, "Vendor: %s", vendor);
    text_draw_raised (textbuf, 772.0f, height - 16.0f, 0.45f);
    sprintf(textbuf, "Renderer: %s", renderer);
    text_draw_raised (textbuf, 772.0f, height - 32.0f, 0.45f);
    sprintf(textbuf, "Avg. frame time: %f ms", timer->frameTime);
    text_draw_raised (textbuf, 772.0f, height - 80.0f, 0.45f);

    /* Debug CPU and RAM */
    sprintf(textbuf, "PC: $%04x %02x %s Ticks: %d, Cycles: %ld, s: %.3f", 
        cpu->lastpc, cpu->opcode, cpu->lastop, cpu->clockticks, cpu->clockCount,
        (float)(NES.ppu.frame / 60.0f));
    text_draw_raised (textbuf, 772.0f, height - 64.0f, 0.45f);

    /* CPU registers and storage locations for program/vars */
    draw_debug_cpu(772.0f, height - 112.0f);
    //draw_debug_ram(12, height - 24,  16, 16, 0x0);
    //draw_debug_ram(12, height - 336, 16, 16, 0x8000);
}

#endif