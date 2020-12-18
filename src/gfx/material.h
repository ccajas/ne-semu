#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <math.h>
#include <string.h>
#include "texture.h"

enum RenderProperty
{
    RENDER_DEFAULT_NORMALS = 1,
    RENDER_CULL_FACE = 2
};

/* Forward declaration */

typedef struct Shader_struct Shader;

typedef struct MaterialProperty_struct
{
    char     propertyName[128];
    uint32_t value;
}
MaterialProperty;

typedef struct Material_struct
{
    char     name[128];
    uint32_t colorDiffuse, colorSpecular, reflectance;
    uint8_t  roughness, metallic, emissive;
    uint8_t  renderProperties;
    
    /* Shader reference */
    Shader*  shader;

    /* Texture data */
    uint32_t textures[TEXTURE_TYPES];
    char     textureNames[TEXTURE_TYPES][128];
}
Material;

inline Material material_init (const char* name)
{
    Material mat = {
        .colorSpecular = 0x221108ff,
        .colorDiffuse  = 0xa0a4a8ff,
        .reflectance   = 0x0a0a0aff,
        .roughness     = 100,
        .metallic      = 12,
        .renderProperties = 0 & RENDER_CULL_FACE,
        .shader = NULL
    };
    strcpy(mat.name, name);
    return mat;
}

inline uint32_t material_int32_from_token (char** rest)
{
    float mValue[3];
    for (int i = 0; i < 3; i++) {
        mValue[i] = atof(strtok_r(NULL, " \n\r\t", rest));
    }
    /* Diffuse color value */
    uint32_t val = ((uint32_t)(mValue[0] * 255) << 24) + 
        ((uint32_t)(mValue[1] * 255) << 16) + ((uint32_t)(mValue[2] * 255) << 8);
    return val;
}

inline uint8_t material_roughness_from_spec (float spec)
{
    /* Derive roughness from Blinn-Phong distribution */
    float roughness = sqrt(2 / (spec + 2));
    return (uint8_t)(roughness * 255);
}

inline uint8_t material_roughness_from_spec_token (char** rest)
{
    float specular = atof(strtok_r(NULL, " \n\t\r", rest));
    return material_roughness_from_spec(specular);
}

inline uint8_t material_metallic_from_token (char** rest)
{
    return (uint8_t)(atof(strtok_r(NULL, " \n\t\r", rest)) * 255);
}

inline void material_illum_from_token (Material * const material, char * illumValue)
{
    /* Illumination value determines use of specular as workflow */
    uint8_t illum = strtol(illumValue, NULL, 10);

    if (illum == 3) {
        material->metallic = (uint8_t)(material->colorSpecular >> 24);
    }
}

inline void material_alpha_from_token (Material * const material, char * alphaValue)
{
    /* Dissolve/alpha transparency value */
    float mAlpha = atof(alphaValue);
    uint8_t alpha = (uint8_t)(mAlpha * 255);
    if (alpha == 0) { alpha = 255; }
    
    material->colorDiffuse += alpha;
}

#endif