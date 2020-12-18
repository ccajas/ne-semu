#include "gl_gen.h"
#include "render.h"

/* Include render.h in only one source file */

uint32_t quadVAO = 0;
uint32_t quadVBO = 0;

float quadVertices[] = {

    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f,-1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 0.0f, 1.0f, 0.0f,
};

void draw_lazy_quad()
{
    if (quadVAO == 0)
    {
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 
    }

    glBindVertexArray(quadVAO);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

uint32_t tetraVAO = 0;
uint32_t tetraVBO = 0;

float tetra_vertices[] = {
    -1.0f, -1.0f, -1.0f, 
    -1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f, -1.0f
};

uint16_t tetra_indices[] = {
    0, 1, 3,
    2, 3, 1,
    3, 2, 0,
    2, 1, 0
};

inline void draw_lazy_tetra() 
{
    if (tetraVAO == 0)
    {
        glGenVertexArrays(1, &tetraVAO);
        glGenBuffers(1, &tetraVBO);
        glBindVertexArray(tetraVAO);

        glBindBuffer(GL_ARRAY_BUFFER, tetraVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tetra_vertices), tetra_vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 
    }

    glBindVertexArray(tetraVAO);
    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, &tetra_indices);
    glBindVertexArray(0);
}

uint32_t cubeVAO = 0;
uint32_t cubeVBO = 0;

void draw_lazy_cube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        uint8_t offset = 0;
        uint8_t stride = 8;
        // Add vertex attributes
        vertex_attribute (GL_TRUE, 0, 3, stride, &offset);
        vertex_attribute (GL_TRUE, 1, 3, stride, &offset);
        vertex_attribute (GL_TRUE, 2, 2, stride, &offset);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void set_frame_render_buffers(uint32_t *fbo, uint32_t* rbo, uint16_t const mapSizeX, uint16_t const mapSizeY)
{
    glGenFramebuffers(1, fbo);
    glGenRenderbuffers(1, rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mapSizeX, mapSizeY);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *rbo);   
}

uint32_t draw_quad_to_framebuffer(
    Shader const *shader,
    uint16_t const mapSizeX, uint16_t const mapSizeY)
{
    /* Generate a 2D LUT from the BRDF equations used */
    unsigned int framebufferTexture = texture_new_blank (GL_CLAMP_TO_EDGE, GL_RG16F, GL_RG, 512, 512);
    //glActiveTexture(GL_TEXTURE0);

    unsigned int fbo, rbo;
    set_frame_render_buffers (&fbo, &rbo, mapSizeX, mapSizeY);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    glViewport(0, 0, mapSizeX, mapSizeY);
    glUseProgram(shader->program);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_lazy_quad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);

    return framebufferTexture;
}

uint32_t draw_cubemap_to_framebuffer (
    Shader const * shader, 
    mat4x4 const * captureViews, 
    uint32_t framebufferTexture, 
    uint16_t mapSize)
{
    mat4x4 capProjection;
    mat4x4_perspective(capProjection, 90.0f * (M_PI / 180.0f), 1.0f, 0.1f, 10.0f);

    unsigned int cubemapTexture = texture_new_cubemap (GL_CLAMP_TO_EDGE, mapSize);

    /* Attempt to bind to cubemap. If invalid, use regular 2D texture */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, framebufferTexture);
    if (glGetError() == GL_INVALID_OPERATION) {
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    }

    glUseProgram (shader->program);
    shader_apply (shader, "projection", capProjection);

    unsigned int fbo, rbo;
    set_frame_render_buffers (&fbo, &rbo, mapSize, mapSize);
    glViewport(0, 0, mapSize, mapSize);

    for (unsigned int i = 0; i < 6; i++)
    {
        shader_apply (shader, "view", captureViews++);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapTexture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_lazy_cube();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);

    return cubemapTexture;
}

uint32_t draw_prefiltermap_to_framebuffer (
    Shader const *shader,
    mat4x4 const *captureViews,
    uint32_t cubemapTexture,
    uint16_t mapSize)
{
    mat4x4 captureProjection;
    mat4x4_perspective(captureProjection, 90.0f * (M_PI / 180.0f), 1.0f, 0.1f, 10.0f);

    unsigned int prefilterTexture  = texture_new_cubemap_linear (GL_CLAMP_TO_EDGE, 128);

    glUseProgram(shader->program);
    shader_apply_int(shader, "environmentMap", 0);
    shader_apply (shader, "projection", captureProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    unsigned int fbo, rbo;
    set_frame_render_buffers (&fbo, &rbo, mapSize, mapSize);

    uint8_t maxMipLevels = 5;
    for (uint8_t mip = 0; mip < maxMipLevels; ++mip)
    {
        // resize framebuffer according to mip-level size.
        uint16_t mipWidth = mapSize * pow(0.5, mip);
        uint16_t mipHeight = mapSize * pow(0.5, mip);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        shader_apply_1f (shader, "roughness", roughness);
        
        for (int i = 0; i < 6; ++i)
        {
            shader_apply (shader, "view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterTexture, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            draw_lazy_cube();
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
    //texture_free(cubemapTexture, 1);
    //texture_free(prefilterTexture, 1);

    return prefilterTexture;
}