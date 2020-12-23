#ifndef _TEXT_H
#define _TEXT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../utils/linmath.h"
#include "gl_gen.h"
#include "shader.h"

#define TOTAL_CHARS     128
#define CHAR_PIXEL_SIZE 32

struct Character 
{
    GLuint TextureID;   // ID handle of the glyph texture
    float px, py;       // Location of glyph in atlas
    float size[2];      // Size of glyph
    float bearing[2];   // Offset from baseline to left/top of glyph
    GLuint advance;     // Horizontal offset to advance to next glyph
}
characters[TOTAL_CHARS];

GLuint textVAO, textVBO;
Shader textShader;

void text_init();

void text_begin(
    int scrWidth,
    int scrHeight);

void text_draw_color (
    const char text[],
    float x,
    float y,
    float scale,
    vec4 color);

inline void text_draw_black (const char text[], float x, float y, float scale)
{
    text_draw_color (text, x, y, scale, (vec4){ 0.0f, 0.0f, 0.0f, 1.0f });
}

inline void text_draw_white (const char text[], float x, float y, float scale)
{
    text_draw_color (text, x, y, scale, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
}

inline void text_draw (const char text[], float x, float y, float scale, const uint32_t color)
{
    float r = (float)((color >> 16) & 0xff) / 255;
    float g = (float)((color >> 8)  & 0xff) / 255;
    float b = (float)(color & 0xff) / 255;  
    text_draw_color (text, x, y, scale, (vec4){ r, g, b, 1.0f });
}

inline void text_draw_alpha (const char text[], float x, float y, float scale, const uint32_t color)
{
    float a = (float)((color >> 24) & 0xff) / 255; 
    float r = (float)((color >> 16) & 0xff) / 255;
    float g = (float)((color >> 8)  & 0xff) / 255;
    float b = (float)(color & 0xff) / 255;  
    text_draw_color (text, x, y, scale, (vec4){ r, g, b, a });
}

inline void text_draw_raised (const char* text, float x, float y, float scale, uint32_t color)
{
    if (color == -1) color = 0xffffff;

    text_draw_black (text, x-1, y-2.5 * scale, scale);
    text_draw (text, x, y, scale, color);
}

#endif