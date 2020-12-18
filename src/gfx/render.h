#ifndef RENDER_H
#define RENDER_H

#include "model.h"
#include "mesh.h"
#include "material.h"
#include "shader.h"

/* Include render.h in only one source file */

extern uint32_t quadVAO;
extern uint32_t quadVBO;

extern uint32_t tetraVAO;
extern uint32_t tetraVBO;

extern uint32_t cubeVAO;
extern uint32_t cubeVBO;

extern float quadVertices[];
extern float tetra_vertices[];
extern uint16_t tetra_indices[];

void draw_lazy_quad();
void draw_lazy_tetra();
void draw_lazy_cube();
void draw_lazy_sphere();

uint32_t draw_quad_to_framebuffer(
    Shader const * shader,
    uint16_t const mapSizeX,
    uint16_t const mapSizeY);

uint32_t draw_cubemap_to_framebuffer (
    Shader const * shader, 
    mat4x4 const * captureViews,
    uint32_t cubemapTexture, 
    uint16_t mapSize);

uint32_t draw_prefiltermap_to_framebuffer (
    Shader const * shader,
    mat4x4 const * captureViews,
    uint32_t cubemapTexture,
    uint16_t mapSize);

/* Inline functions */

inline void vertex_attribute (
    int numElements, 
    uint8_t arrayIndex, 
    int numComponents, 
    uint8_t stride, 
    uint8_t* offset)
{
    if (numElements) 
    {
        glEnableVertexAttribArray(arrayIndex);
        glVertexAttribPointer(arrayIndex, numComponents, GL_FLOAT, GL_FALSE, stride * sizeof(float), 
            (void*)(*offset * sizeof(float)));
        *offset += numComponents;
    }
}

/* Model mesh drawing functions */

inline void model_mesh_draw (Model* const model, int index, const uint32_t matID)
{
    Material* material = &model->materials[matID];
    if (material->shader == NULL) {
        return;
    }

    uint32_t offset = model->meshes[index].indexOffset;
    uint32_t count  = (index + 1 == model->meshCount) ? 
        model->indices.total - offset : 
        model->meshes[index + 1].indexOffset - offset;
    
    shader_apply_int (material->shader, "_albedo",   material->colorDiffuse);
    shader_apply_int (material->shader, "_fresnel0", material->reflectance);
    shader_apply_int (material->shader, "renderProperties", material->renderProperties);

    for (int type = 0; type <= TEXTURE_EMISSIVE; type++) 
    {
        glActiveTexture(GL_TEXTURE0 + type);
        glBindTexture (GL_TEXTURE_2D, material->textures[type]);      
    }
    /* Switch on/off backface culling depending on material property */
    if (matID > 0) 
    {
        Material* lastMaterial = &model->materials[matID-1];
        if ((lastMaterial->renderProperties & RENDER_CULL_FACE) != 
            (material->renderProperties & RENDER_CULL_FACE)) 
        {
            if ((material->renderProperties & RENDER_CULL_FACE) == RENDER_CULL_FACE) {
                glEnable (GL_CULL_FACE);
            }
            else {
                glDisable (GL_CULL_FACE);
            }
        }
    }

    glDrawElements (GL_TRIANGLES, count, GL_UNSIGNED_INT, &model->indices.data[offset]);
}

inline void model_draw (Model* const model)
{
    if (model->materialCount == 0) { return; }

    Shader* shader = model->materials[0].shader;

    shader_apply_int (shader, "albedoMap",    TEXTURE_DIFFUSE);
    shader_apply_int (shader, "normalMap",    TEXTURE_NORMAL);
    shader_apply_int (shader, "alphaMap",     TEXTURE_ALPHA);
    shader_apply_int (shader, "metallicMap",  TEXTURE_METALLIC);
    shader_apply_int (shader, "roughnessMap", TEXTURE_ROUGHNESS);
    shader_apply_int (shader, "emissiveMap",  TEXTURE_EMISSIVE);
    shader_apply_int (shader, "aoMap",        TEXTURE_AO);
    
    if (model->materialCount > 0) 
    {
        glActiveTexture(GL_TEXTURE0 + TEXTURE_MATCAP);
        glBindTexture (GL_TEXTURE_2D, model->materials[0].textures[TEXTURE_MATCAP]);   
    }

    for (int i = 0; i < model->meshCount; i++)
    {
        uint32_t matID  = model->meshes[i].materialID;
        uint32_t color  = model->materials[matID].colorDiffuse;

        if ((color & 255) == 255) {
            model_mesh_draw (model, i, matID);
        }
    }
}

inline void model_draw_transparent (Model* const model)
{
    Shader* shader = NULL;
    if (model->materialCount > 0) {
        shader = model->materials[0].shader;
    }

    if (shader == NULL) {
        return;
    }
    glUseProgram (shader->program);

    for (int i = 0; i < model->meshCount; i++)
    {
        uint32_t matID  = model->meshes[i].materialID;
        uint32_t color  = model->materials[matID].colorDiffuse;

        if ((color & 255) < 255) {
            model_mesh_draw (model, i, matID);
        }
    }
    glUseProgram(0);
}

inline void draw_lazy_model (Model* const model)
{
    // initialize (if necessary)
    if (model->VAO == 0 && model->indices.total > 0)
    {
        printf("Building meshModel (%u triangles, %u vertices)... \n", model->indices.total / 3, model->vertices.total / 3);
        printf("Materials: %u meshes: %u \n", model->materialCount, model->meshCount);
        GLuint modelVB;
        glGenVertexArrays(1, &model->VAO);
        glGenBuffers(1, &modelVB);
        glBindVertexArray(model->VAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, modelVB);
        glBufferData(GL_ARRAY_BUFFER, model->vertices.total * sizeof(float), model->vertices.data, GL_STATIC_DRAW);

        uint8_t offset = 0;
        uint8_t stride = 6 + (model->texcoordCount ? 2 : 0);
        // Add vertex attributes
        vertex_attribute (model->positionCount, 0, 3, stride, &offset);
        vertex_attribute (model->normalCount,   1, 3, stride, &offset);
        vertex_attribute (model->texcoordCount, 2, 2, stride, &offset);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render model
    glPolygonMode(GL_FRONT_AND_BACK, model->drawMode == DRAW_LINE ? GL_LINE : GL_FILL);
    glBindVertexArray(model->VAO);
    model_draw (model);

    glDepthMask(GL_TRUE);
	model_draw_transparent (model);
    
    // Change draw state only if we need to
    if (model->drawMode == DRAW_LINE)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
	glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

#endif