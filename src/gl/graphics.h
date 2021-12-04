#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "../glfw/timer.h"
#include "../bus.h"
#include "../palette.h"
#include "../utils/linmath.h"
#include "gl_gen.h"
#include "shader.h"

#define PPU_DEBUG

typedef struct Scene_struct
{
    uint8_t bgColor[3];

    GLuint 
        fbufferTexture, 
        pTableTexture, 
        nameTableTexture,
        paletteTexture;
    Shader 
        fbufferShader,
        debugShader;
}
Scene;

extern const char * default_fs_source;
extern const char * ppu_vs_source;
extern const char * ppu_fs_source;

extern uint32_t quadVAO[2];

void draw_lazy_quad (const float width, const float height, const int i);
void draw_scene     (GLFWwindow *, Scene * const);

inline void texture_setup (uint32_t * const textureID, uint16_t width, uint16_t height, GLenum filter, const void * data)
{
    glGenTextures (1, textureID);
    glBindTexture(GL_TEXTURE_2D, *textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

/* inline void graphics_init (Scene * const); */

#ifdef CPU_DEBUG

void draw_debug (GLFWwindow * window, Timer * const timer);

inline void draw_debug_cpu (int32_t const x, int32_t const y)
{
    char textbuf[256];
    const float size = 0.5f;

    text_draw_raised ("STATUS", x, y, size, 0xffee00);

    sprintf (textbuf, "X:$%02x [%03d], Y:$%02x [%03d]", cpu->r.x, cpu->r.x, cpu->r.y, cpu->r.y);
    text_draw_raised (textbuf, x , y - 16, size, -1);
    sprintf (textbuf, "A:$%02x, [%03d], SP:$%04x", cpu->r.a, cpu->r.a, cpu->r.sp);
    text_draw_raised (textbuf, x , y - 32, size, -1);
}

inline void draw_debug_ram (int32_t const x, int32_t const y, int8_t rows, int8_t cols, int16_t const start)
{
    char textbuf[256];
    const float size = 0.45f;
    int16_t addr = start;

    sprintf(textbuf, " ---- ");
    for (int j = 0; j < cols; j++)
    {
        sprintf(textbuf + strlen(textbuf), "%02x ", j);
    }
    text_draw_raised (textbuf, x, y, size, -1);
    for (int i = 1; i <= rows; i++)
    {
        sprintf(textbuf, "$%04x ", (uint16_t)addr);
        for (int j = 0; j < cols; j++)
        {
            sprintf(textbuf + strlen(textbuf), "%02x ", bus_read(&NES, addr++)); 
        }
        text_draw_raised (textbuf, x, y - (i * 16), size, -1);
    }
}

#endif

#ifdef PPU_DEBUG

void draw_ntable_debug (GLFWwindow * const, Scene * const);
void draw_ptable_debug (GLFWwindow * const, Scene * const);
void draw_debug_tiles  (int32_t const width, int32_t const height);

#endif

#endif