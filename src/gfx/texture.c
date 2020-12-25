#include <stdint.h>
#include "texture.h"

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