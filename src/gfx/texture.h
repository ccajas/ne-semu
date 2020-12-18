#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "gl_gen.h"

enum TextureType 
{
    TEXTURE_DIFFUSE = 0,
    TEXTURE_NORMAL,
    TEXTURE_SPECULAR,
    TEXTURE_ALPHA,
    TEXTURE_METALLIC,
    TEXTURE_ROUGHNESS,
    TEXTURE_AO,
    TEXTURE_EMISSIVE,
    TEXTURE_REFLECTANCE,
    TEXTURE_AMBIENT,
    TEXTURE_PREFILTER,
    TEXTURE_BRDF,
    TEXTURE_MATCAP,
    TEXTURE_TYPES
}
TextureType;

/* Default colors for various texture types */
const unsigned char defaultColors[8][4];

void texture_init (
    uint32_t const textureID, 
    GLenum wrapParam,
    GLenum minFilter);

void texture_init_cubemap (
    uint32_t const textureID, 
    GLenum wrapParam, 
    uint32_t size);

uint32_t texture_new (
    const char* textureName, 
    GLenum wrapParam);

uint32_t texture_new_blank (
    GLenum wrapParam, 
    GLenum internalFormat,
    GLenum format,
    uint32_t sizeX,
    uint32_t sizeY);
    
uint32_t texture_new_HDR (
    const char* textureName, 
    float* (*textureReadCallback)());

uint32_t texture_new_cubemap (
    GLenum wrapParam, 
    uint32_t size);

uint32_t texture_new_cubemap_linear (
    GLenum wrapParam, 
    uint32_t size);

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

inline void texture_set_cubemap (uint32_t texture, uint32_t texUnit)
{
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
}

inline uint32_t texture_new_color (unsigned char const defaultColor[])
{
    uint32_t textureID = 0;
    texture_create (&textureID);
    texture_init (textureID, GL_CLAMP_TO_EDGE, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultColor);

    return textureID;
}

inline uint32_t texture_new_2D_array (unsigned char const pixelData[], int const depth)
{
    uint32_t textureID = 0;
    texture_create (&textureID);

    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 
        1, 1, depth, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        pixelData
    );

    return textureID;
}

#endif