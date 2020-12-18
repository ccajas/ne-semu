#ifndef SIMPLE_MODEL_H
#define SIMPLE_MODEL_H

#include <limits.h>
#include "../utils/linmath.h"
#include "../utils/defs.h"
#include "../utils/v_array.h"
#include "material.h"

#define max(a, b) ( \
    { __auto_type _a = (a); __auto_type _b = (b); \
    _a > _b ? _a : _b; })

#define min(a, b) ( \
    { __auto_type _a = (a); __auto_type _b = (b); \
    _a < _b ? _a : _b; })

typedef struct Mesh_struct Mesh;

enum DrawMode 
{
    DRAW_LINE = 0,
    DRAW_FILL
};

typedef struct Model_struct
{
    /* Temp storage for model file contents */
    unsigned char* fileBuf;
    unsigned char pathName[512];

    /* Vertex Array Object this model is a part of */
    uint32_t VAO;

    /* Dynamic arrays to store vertices and indices */
    struct Varray vertices, indices;

    /* Counts of various attributes */
    uint32_t positionCount, texcoordCount, normalCount;
    uint32_t faceCount, materialCount, meshCount;

    /* Dynamic array of meshes/materials and pointer to current */
    Mesh     *meshes;
    Material *materials;
    enum DrawMode drawMode;

    /* Min and max bounds for scaling */
    vec3  center;
    float scale;
}
Model;

void model_create_mesh (Model *const model, const char* name, const uint32_t offset);
void model_create_material (Model *const model, const char* name);
void model_assign_texture (Model *const model, enum TextureType type, const char* name);
void model_assign_shader (Model *const model, Shader* const shader);

inline void model_init (Model *const model)
{
    /* Set default values */
    model->VAO = 0;
    model->scale = 0;
    model->faceCount   = model->positionCount = 0;
    model->normalCount = model->texcoordCount = 0;
    model->meshCount = model->materialCount = 0;
    model->drawMode = DRAW_FILL;

    va_init (&model->vertices, 1);
    va_init (&model->indices, 1);
}

static inline void triangle_calculate_normal (vec3 tri[3], vec3* normal)
{
    vec3 u, v;

    vec3_add (u, tri[1], tri[0]);
    vec3_sub (v, tri[2], tri[0]);

    *normal[0] = u[1] * v[2] - u[2] * v[1];
    *normal[1] = u[2] * v[0] - u[0] * v[2];
    *normal[2] = u[0] * v[1] - u[1] * v[0];
}

static inline void model_add_normals (Model* const model)
{
    if (model->normalCount) { return; }

    for (int i = 0; i < model->indices.total; i += 3)
    {
        //printf("%d \n", i);
        int points[3] = {
            va_get_int (&model->indices, i), 
            va_get_int (&model->indices, i+1), 
            va_get_int (&model->indices, i+2)
        };

        /* Get the three vertex positions first */
        vec3 verts[3];
        for (int j = 0; j < 3; j++)
        {
            verts[j][0] = model->vertices.data[points[j] * 3].f;
            verts[j][1] = model->vertices.data[points[j] * 3 + 1].f;
            verts[j][2] = model->vertices.data[points[j] * 3 + 2].f;
        };

        //printf("%f %f %f \n", verts[0][0], verts[0][1], verts[0][2]);

        vec3 normal;
        triangle_calculate_normal (verts, &normal);
    }
}

static inline void model_center_and_scale (Model* const model)
{
    /* Get minimum and maximum position values */
    vec3 bbox_min, bbox_max;

    /* Find min and max bounds for scale */
    for (int i = 0; i < 3; i++) 
    {
        bbox_min[i] = INT_MAX;
        bbox_max[i] = INT_MIN;
        model->center[i] = 0;  
    }

    for (int i = 0; i < model->positionCount; i++)
    {
        int v = i * (model->texcoordCount ? 8 : 6);
        for (int j = 0; j < 3; j++)
        {
            bbox_min[j] = min (bbox_min[j], model->vertices.data[v+j].f);
            bbox_max[j] = max (bbox_max[j], model->vertices.data[v+j].f);
        }
    }

    /* Calculate the center */
    vec3_add   (model->center, bbox_min, bbox_max);
    vec3_scale (model->center, model->center, 0.5);

    for (int i = 0; i < 3; i++) {
        model->scale = max (bbox_max[i] - bbox_min[i], model->scale);
    }
}

#define copy_tex_references(a, b, texID) ( \
    { int _texID = texID; if (!strcmp(a, b)) { \
        strcpy(a, material->textureNames[type]); material->textures[type] = _texID; \
        textureMatchFound = 1; break; \
    }})

inline void model_load_textures (Model* const model)
{
    /* Loop materials > loop texture references for each material */
    for (int i = 0; i < model->materialCount; i++)
    {
        Material* material = &model->materials[i];

        for (int type = TEXTURE_DIFFUSE; type <= TEXTURE_EMISSIVE; type++) 
        {
            char* textureName = material->textureNames[type];

            if (!is_empty(textureName)) 
            {
                int textureMatchFound = 0;

                /* Search for existing textures in this material and copy references if found */
                for (int t = 0; t < type; t++)
                {
                    char* name = material->textureNames[t];
                    copy_tex_references(name, textureName, material->textures[t]);
                }

                /* Search for existing textures in other materials */
                for (int j = 0; j < i; j++) 
                {
                    char* name = model->materials[j].textureNames[type];
                    copy_tex_references(name, textureName, model->materials[j].textures[type]);
                }
                if (!textureMatchFound) {
                    material->textures[type] = texture_new (textureName, GL_REPEAT);
                }
            }
            /* Set default color */
            else {
                texture_create (&material->textures[type]);
                unsigned char defaultColor[4];
                memcpy(&defaultColor, defaultColors[type], 4);
                
                if (type == TEXTURE_METALLIC || type == TEXTURE_ROUGHNESS) {
                    defaultColor[1] = (char)(material->roughness); // green channel
                    defaultColor[2] = (char)(material->metallic);  // blue channel
                }
                material->textures[type] = texture_new_color (defaultColor);
                if (type == TEXTURE_NORMAL) {
                    material->renderProperties |= RENDER_DEFAULT_NORMALS;
                }
            }
        }
    }
}

inline void model_toggle_wireframe (Model* const model) 
{
    model->drawMode = (model->drawMode == DRAW_LINE) ? DRAW_FILL : DRAW_LINE;
}

inline void model_free(Model* const model)
{
    va_free (&model->vertices);
    va_free (&model->indices);

    /* Delete associated textures from memory */
    for (int i = 0; i < model->materialCount; i++)
    {
        for (int type = TEXTURE_DIFFUSE; type <= TEXTURE_EMISSIVE; type++) 
        {
            uint32_t texture = model->materials[i].textures[type];
            texture_free (texture, 1);
        }
    }

    /* Set default values */
    model->VAO = model->positionCount = 0;
    model->normalCount = model->texcoordCount =  0;
    model->meshCount   = model->materialCount = 0;
}

#endif