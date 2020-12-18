#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "model_parsers.h"
#include "render.h"
#include "text.h"

typedef struct Camera_struct Camera;

typedef struct Scene_struct
{
    uint8_t bgColor[3];
    Model   model;
    Shader  mainShader, skyboxShader;
    Camera *camera;
}
Scene;

inline void graphics_init()
{
    gladLoadGL();
    glfwSwapInterval(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	/* Init text and framebuffer objects */
	text_init();
	//framebuffer_init_ms (1280, 720, 8);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL); // Backdrop depth trick

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

static inline void draw_scene (GLFWwindow * window, Scene * scene, Camera * camera)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4x4 model, view, proj;
    //mat4x4_look_at(view, camera->pos, camera->target, camera->up);

    glUseProgram (scene->mainShader.program);

    // Camera projection
    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    //mat4x4_perspective(proj, camera->fov * (M_PI / 180.0f), (float) width / (float) height, 0.1f, 100.0f);

    // get matrix uniform locations and set values
    shader_apply (&scene->mainShader, "view", view);
    shader_apply (&scene->mainShader, "projection", proj);
    //shader_apply_3f (&scene->mainShader, "CamPos", camera->pos);
    
    shader_apply_int (&scene->mainShader, "brdfLUT",       TEXTURE_BRDF);
    shader_apply_int (&scene->mainShader, "irradianceMap", TEXTURE_AMBIENT);
    shader_apply_int (&scene->mainShader, "prefilterMap",  TEXTURE_PREFILTER);

    mat4x4_identity (model);
    mat4x4_scale (model, model, 1.0f / scene->model.scale);
    mat4x4_translate_in_place (model, -scene->model.center[0], -scene->model.center[1], -scene->model.center[2]);
    shader_apply (&scene->mainShader, "model", model);

    glEnable(GL_CULL_FACE);
    draw_lazy_model (&scene->model);
}

#endif