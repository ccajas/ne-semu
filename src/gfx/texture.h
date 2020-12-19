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

uint32_t texture_new_blank (
    GLenum wrapParam, 
    GLenum internalFormat,
    GLenum format,
    uint32_t sizeX,
    uint32_t sizeY);

/* Inline functions */

inline void texture_create (uint32_t* const textureID)
{
    glGenTextures (1, textureID);
}

inline void texture_free (uint32_t texture, int const size)
{
    glDeleteTextures (1, &texture);
}

inline void texture_store_DDS (unsigned char** data, uint32_t width, uint32_t height, uint8_t numMipmaps)
{
    int offset = 0;
    for (int i = 0; i < numMipmaps && (width || height); i++)
    {    
        width  = (width  == 0) ? 1 : width;
        height = (height == 0) ? 1 : height;    
        int size = ((width+3) >> 2) * ((height+3) >> 2) * 8;  // blockSize;    

        glCompressedTexImage2D (GL_TEXTURE_2D, i, 0x83f0, // ddsimage->format, 
            width, height, 0, size, *data + offset);
        
        offset += size;    
        width  >>= 1;    
        height >>= 1;  
    }
}

inline void texture_set (uint32_t texture, uint32_t texUnit)
{
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(GL_TEXTURE_2D, texture);
}

inline uint32_t texture_new_color (unsigned char const defaultColor[])
{
    uint32_t textureID = 0;
    texture_create (&textureID);
    texture_init (textureID, GL_CLAMP_TO_EDGE, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultColor);

    return textureID;
}

#endif