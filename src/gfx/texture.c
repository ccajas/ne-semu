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

void texture_init (uint32_t const textureID, GLenum wrapParam, GLenum minFilter)
{
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void texture_init_cubemap (uint32_t const textureID, GLenum wrapParam, uint32_t size)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, NULL);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrapParam);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrapParam);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrapParam);
}

uint32_t texture_new_cubemap (GLenum wrapParam, uint32_t size)
{
    uint32_t textureID = 0;
    texture_create (&textureID);
    texture_init_cubemap (textureID, wrapParam, size);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

uint32_t texture_new_cubemap_linear (GLenum wrapParam, uint32_t size)
{
    uint32_t textureID = 0;
    texture_create (&textureID);
    texture_init_cubemap (textureID, wrapParam, size);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return textureID;
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

uint32_t texture_new_HDR (const char* textureName, float* (*textureReadCallback)())
{
    int width, height, nrComponents;
    uint32_t textureID = 0;
    float *data = textureReadCallback (textureName, &width, &height, &nrComponents, 0);

    if (data)
    {
        texture_create (&textureID);
        texture_init (textureID, GL_CLAMP_TO_EDGE, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); 

        //stbi_image_free(data);
        free(data);
    }
    else {
        e_printf("%s", "Failed to load HDR image\n");
    }
    return textureID;
}