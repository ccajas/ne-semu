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
        paletteTexture;
    Shader fbufferShader;
}
Scene;

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

inline void graphics_init (Scene * const scene)
{
    gladLoadGL();
    glfwSwapInterval(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	/* Create shader */
	scene->fbufferShader = shader_init_source (ppu_vs_source, ppu_fs_source);

	/* Init text and texture objects */
	//text_init ();

	char* pixels = (char*)calloc(192, 1);
	for (int i = 0; i < 192; i += 3)
	{
		uint16_t palColor = palette2C03[i / 3];
		pixels[i] =   (palColor >> 8) << 5;
		pixels[i+1] = (palColor >> 4) << 5;
		pixels[i+2] = (palColor << 5);
	}
	free (pixels);

	/* Create main textures */
    texture_setup (&scene->fbufferTexture, 256, 240, GL_NEAREST, NULL);
    texture_setup (&scene->pTableTexture,  256, 256, GL_NEAREST, NULL);
    texture_setup (&scene->paletteTexture, 64,  1, GL_NEAREST, pixels);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);
}

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

void draw_ppu_debug   (GLFWwindow * const, Scene * const );
void draw_debug_tiles (int32_t const width, int32_t const height);

#endif

#endif