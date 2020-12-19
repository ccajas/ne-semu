#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
//#define USE_HDR

#if defined (USE_HDR)
#include "utils/stb_image.h"
#endif

#include "app.h"

Material setup_PBR (uint32_t mapSize)
{
    Material material = material_init("defaultPBR");

    Shader irradianceShader = shader_init ("./assets/shaders/cubemap.vs", "./assets/shaders/irradiance-conv.fs");
    Shader cubemapShader    = shader_init ("./assets/shaders/cubemap.vs", "./assets/shaders/cubemap-from-equirect.fs");
    Shader prefilterShader  = shader_init ("./assets/shaders/cubemap.vs", "./assets/shaders/prefilter.fs");
    Shader brdfShader       = shader_init ("./assets/shaders/brdf-lut.vs", "./assets/shaders/brdf-lut.fs");

    /* Set up projection and view matrices for capturing data onto the 6 cubemap face directions */
    mat4x4 captureViews[6];
    for (int i = 0; i < 6; i++) {
        mat4x4_identity(captureViews[i]);
    }

    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 left = {-1.0f,0.0f, 0.0f},
        right = {1.0f, 0.0f, 0.0f},
        up =    {0.0f, 1.0f, 0.0f},
        down =  {0.0f,-1.0f, 0.0f},
        back =  {0.0f, 0.0f,-1.0f},
        front = {0.0f, 0.0f, 1.0f};

    mat4x4_look_at (captureViews[0], center, right, down);
    mat4x4_look_at (captureViews[1], center, left, down);
    mat4x4_look_at (captureViews[2], center, up, front);
    mat4x4_look_at (captureViews[3], center, down, back);
    mat4x4_look_at (captureViews[4], center, front, down);
    mat4x4_look_at (captureViews[5], center, back, down);

    /* PBR: Framebuffer setup */

    /* Setup cubemap texture */
#if defined (USE_HDR)
    stbi_set_flip_vertically_on_load (GL_TRUE);
    unsigned int hdrTexture = texture_new_HDR ("./assets/textures/hdr/Topanga_Forest_B_3k.hdr", stbi_loadf);
#else
    unsigned int hdrTexture = texture_new ("./assets/textures/hdr/canyon.bmp", GL_CLAMP_TO_EDGE);
#endif
    /* Generate cubemap from HDR texture using equirectangular projection */      
    unsigned int envCubemap    = draw_cubemap_to_framebuffer (&cubemapShader,   &captureViews[0], hdrTexture, mapSize);
    unsigned int irradianceMap = draw_cubemap_to_framebuffer (&irradianceShader, &captureViews[0], envCubemap, 32);
    unsigned int prefilterMap  = draw_prefiltermap_to_framebuffer (&prefilterShader, &captureViews[0], envCubemap, 128);
    unsigned int brdfLUT = draw_quad_to_framebuffer(&brdfShader, 512, 512);

    material.textures[TEXTURE_AMBIENT]   = irradianceMap;
    material.textures[TEXTURE_PREFILTER] = prefilterMap;
    material.textures[TEXTURE_BRDF]      = brdfLUT;

    texture_set (brdfLUT, TEXTURE_BRDF);
    texture_set_cubemap (irradianceMap, TEXTURE_AMBIENT);
    texture_set_cubemap (prefilterMap,  TEXTURE_PREFILTER);

    return material;
}

static inline void setup_scene (GLFWwindow * window, Scene * scene, Camera * camera, const char* filename)
{
    camera->sensitivity = 240.0f;
    camera->fov    = 45.0;
    camera->up[1]  = 1.0;
    camera->pitch  = 0;
    camera->yaw    = M_PI;

    scene->bgColor[0] = 117;
    scene->bgColor[1] = 119;
    scene->bgColor[2] = 121;

    /* Set up defaults for skybox shader */
    glUseProgram (scene->skyboxShader.program);
    mat4x4 proj;

    int32_t scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport (0, 0, scrWidth, scrHeight);
    mat4x4_perspective(proj, camera->fov * (M_PI / 180.0f), (float) scrWidth / (float) scrHeight, 0.25f, 100.0f);
    shader_apply (&scene->skyboxShader, "projection", proj);

	renderer_update_camera(camera, 1.5);

    /* Finally, load the model */
    //model_load (&scene->model, filename);
    //model_assign_shader (&scene->model, &scene->mainShader);
}

static inline void draw_skybox (Scene * scene, Camera * camera, Material * material)
{
    mat4x4 view;
    mat4x4_look_at(view, camera->pos, camera->target, camera->up);
    glUseProgram (scene->skyboxShader.program);

    shader_apply (&scene->skyboxShader, "view", view);
    texture_set_cubemap (material->textures[TEXTURE_AMBIENT], TEXTURE_DIFFUSE);
        
    glDisable(GL_CULL_FACE);
    draw_lazy_cube();
}

inline void draw_debug_cpu (int32_t const x, int32_t const y)
{
    char textbuf[256];
    const float size = 0.5f;

    text_draw_raised ("STATUS", x, y, size);

    sprintf (textbuf, "PC: $%04x", cpu->r.pc);
    text_draw_raised (textbuf, x , y - 24, size);
    sprintf (textbuf, "A: $%02x [%d]", cpu->r.a, cpu->r.a);
    text_draw_raised (textbuf, x , y - 48, size);
    sprintf (textbuf, "X: $%02x [%d]", cpu->r.x, cpu->r.x);
    text_draw_raised (textbuf, x , y - 72, size);
    sprintf (textbuf, "Y: $%02x [%d]", cpu->r.y, cpu->r.y);
    text_draw_raised (textbuf, x , y - 96, size);
    sprintf (textbuf, "Stack P: $%04x", cpu->r.sp);
    text_draw_raised (textbuf, x , y - 120, size);
}

inline void draw_debug_ram (int32_t const x, int32_t const y, int8_t rows, int8_t cols, int16_t const start)
{
    char textbuf[256];
    const float size = 0.5f;
    int16_t addr = start;

    sprintf(textbuf, " ---- ");
    for (int j = 0; j < cols; j++)
    {
        sprintf(textbuf + strlen(textbuf), "%02x ", j);
    }
    text_draw_raised (textbuf, x, y, size);
    for (int i = 1; i <= rows; i++)
    {
        sprintf(textbuf, "$%04x ", (uint16_t)addr);
        for (int j = 0; j < cols; j++)
        {
            sprintf(textbuf + strlen(textbuf), "%02x ", bus_read(&bus, addr++)); 
        }
        text_draw_raised (textbuf, x, y - (i * 16), size);
    }
}

inline void draw_debug_ppu (int32_t const x, int32_t const y)
{
    ppu_debug (&bus.ppu, x, y, 0);
    ppu_debug (&bus.ppu, x, y, 1);
}

inline void draw_debug (GLFWwindow * window, Timer * timer)
{
    update_timer (timer, glfwGetTime());

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    char textbuf[256];     
    text_begin (width, height);
    const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
    const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model

    sprintf(textbuf, "Vendor: %s Renderer: %s", vendor, renderer);
    text_draw_white (textbuf, 16.0f, 40.0f, 0.75f);
    sprintf(textbuf, "Avg. frame time: %f ms", timer->frameTime);
    text_draw_white (textbuf, 16.0f, 16.0f, 0.75f);

    /* Debug CPU and RAM */
    sprintf(textbuf, "PC: $%04x %02x %s Ticks: %d, Cycles: %ld", cpu->lastpc, cpu->opcode, cpu->lastop, cpu->clockticks, cpu->clockCount);
    text_draw_white (textbuf, 16.0f, 64.0f, 0.75f);

    /* CPU registers and storage locations for program/vars */
    draw_debug_cpu(width - 240, height - 24);
    draw_debug_ram(16, height - 24,  16, 16, 0x0);
    draw_debug_ram(16, height - 336, 16, 16, 0x8000);

    /* Graphical PPU debug */
    draw_debug_ppu(width, height);
}

int main (int argc, char** argv)
{
    App app = {
        .dropPath       = NULL,
        .title          = "ne-semu",
        .onScroll       = glfw_cb_scroll,
        .onDrop         = glfw_cb_drop,
        .onWindowResize = glfw_cb_window_size,
        .resolution     = { 1280, 720 }
    };
    app_init (&app);

    /* Load shaders and model */
    app.scene.mainShader   = shader_init ("./assets/shaders/default.vs", "./assets/shaders/pbr.fs");
    app.scene.skyboxShader = shader_init ("./assets/shaders/skybox.vs", "./assets/shaders/skybox.fs");

    /* Set common parameters */
    //Material pbrm = setup_PBR (512);

    /* Setup camera for first frame */
    Camera camera = {0};
    app.scene.camera = &camera;

    const char* filename = argv[1] ? argv[1] : "default";
    setup_scene (app.window, &app.scene, app.scene.camera, filename);

    /* Main update loop */
    while (app.running)
    {
        app_draw (&app);
        
        /* Measure frame speed */
        draw_debug (app.window, &app.timer);

        app_update (&app);
    }
 
    app_free(&app);
    return 0;
}
 