#include <stdint.h>
#include "image_parsers.h"
#include "texture.h"

/* Default colors for various texture types */
const unsigned char defaultColors[8][4] = 
{
    { 255, 255, 255, 255 }, /* Diffuse */
    { 127, 127, 255, 255 }, /* Normal */
    { 255, 255, 255, 255 }, /* Specular */
    { 255, 255, 255, 255 }, /* Alpha */
    {   0, 100,   0, 255 }, /* MetalRoughness */
    {   0, 100,   0, 255 }, /* MetalRoughness */
    { 255, 255, 255, 255 }, /* AO */
    {   0,   0,   0, 255 }  /* Emissive */
};

void texture_init (uint32_t const textureID, GLenum wrapParam, GLenum filter)
{
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

uint32_t texture_new_blank (
    GLenum wrapParam, 
    GLenum internalFormat,
    GLenum format,
    uint32_t sizeX,
    uint32_t sizeY)
{
    uint32_t textureID = 0;
    texture_create (&textureID);
    texture_init (textureID, wrapParam, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, sizeX, sizeY, 0, format, GL_FLOAT, 0);

    return textureID;
}

uint32_t texture_new (
    const char* textureName, 
    GLenum wrapParam)
{
    uint32_t width = 0, height = 0;
    uint8_t numMipmaps = 0;
    unsigned char* data = NULL;

    uint32_t textureID = 0;
    texture_create (&textureID);
    texture_init (textureID, wrapParam, GL_LINEAR_MIPMAP_LINEAR);
    const char* ext = GET_FILE_EXT (textureName);

    if (!strcmp(ext, "BMP") || !strcmp(ext, "bmp")) 
    {
        data = read_BMP (textureName, &width, &height);
        if (data) 
        {   
#ifndef NDEBUG
            printf("Loading %s \"%s\" \n", ext, textureName);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            free (data);
        }
    }
    else if (!strcmp(ext, "DDS") || !strcmp(ext, "dds")) 
    {
        data = read_DDS (textureName, &width, &height, &numMipmaps);
        if (data) 
        {
#ifndef NDEBUG
            printf("Loading %s \"%s\" \n", ext, textureName);
#endif
            texture_store_DDS (&data, width, height, numMipmaps);
            glGenerateMipmap (GL_TEXTURE_2D);
            free (data);
        }
    }
    else if (!strcmp(ext, "TGA") || !strcmp(ext, "tga")) 
    {
        data = read_TGA (textureName, &width, &height);
        if (data) 
        {   
#ifndef NDEBUG
            printf("Loading %s \"%s\" \n", ext, textureName);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap (GL_TEXTURE_2D);
            free (data);
        }
    }

    return textureID;
}
