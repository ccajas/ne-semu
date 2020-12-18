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

void text_draw (
    const char text[],
    float x,
    float y,
    float scale,
    vec3 color);

inline void text_draw_black (const char text[], float x, float y, float scale)
{
    text_draw (text, x, y, scale, (float[3]){ 0.0f, 0.0f, 0.0f });
}

inline void text_draw_white (const char text[], float x, float y, float scale)
{
    text_draw (text, x, y, scale, (float[3]){ 1.0f, 1.0f, 1.0f });
}

inline void text_draw_raised (const char* text, float x, float y, float scale)
{
    text_draw_black (text, x-1, y-2.5 * scale, scale);
    text_draw_white (text, x, y, scale);
}

#endif