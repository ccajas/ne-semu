#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "gl_gen.h"

void texture_init (
    uint32_t const textureID, 
    GLenum wrapParam,
    GLenum minFilter);

uint32_t texture_new (
    const char* textureName, 
    GLenum wrapParam);

/* Inline functions */

inline void texture_create (uint32_t* const textureID)
{
    glGenTextures (1, textureID);
}

inline void texture_free (uint32_t texture, int const size)
{
    glDeleteTextures (1, &texture);
}

#endif