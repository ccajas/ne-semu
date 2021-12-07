#include "graphics.h"

const char * default_fs_source =

"#version 130\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"

"void main()\n"
"{\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(sampled, 0.7);\n"
"}\n";

const char * ppu_vs_source =

"#version 330\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
"layout (location = 1) in vec2 texture;"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"

"void main()\n"
"{\n"
"    vec4 localPos = model * vec4(vertex.xy, 1.0, 1.0);"
"    gl_Position = projection * vec4(localPos.xy, 0.0, 1.0);\n"
"    TexCoords = texture.xy;\n"
"}\n";
 
const char * ppu_fs_source =

"#version 130\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"
"uniform float time;\n"

"vec3 applyVignette(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy) - vec2(0.5);\n"           
"    float dist = length(position);\n"

"    float radius = 1.3;\n"
"    float softness = 1.0;\n"
"    float vignette = smoothstep(radius, radius - softness, dist);\n"
"    color.rgb = color.rgb - (0.8 - vignette);\n"
"    return color;\n"
"}\n"

"float mod(float x, float y) { return x - (y * floor( x / y )); }\n"

"vec3 applyScanline(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    float px = 1.0/512.0;\n"
"    position.x += mod(position.y * 240.0, 2.0) * px;\n"
"    color *= pow(fract(position.x * 256.0), 0.4);\n"
"    color *= pow(fract(position.y * 240.0), 0.5);\n"
"    return color * 1.67;\n"
"}\n"

"vec3 applyScanline1(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
//"    float px = 1.0/512.0;\n"
//"    position.x += mod(position.y * 240.0, 2.0) * px;\n"
//"    color *= pow(fract(position.x * 256.0 - 128), 1.1);\n"
"    color *= pow(fract(position.y * 240.0 - 120), 0.25);\n"
"    return color * 1.15;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint    = vec3(113, 115, 126) / 127.0;\n"
"    vec2 tx      = TexCoords.xy;\n"
//"    tx.x += sin(tx.y * 960.0) / 1024.0;\n"
"    vec3 sampled = texture2D(indexed, tx).rgb;\n"
//"    sampled      = pow(sampled, vec3(1/1.4));\n"
"    sampled      = applyScanline1(applyVignette(sampled));\n"
"    gl_FragColor = vec4(pow(sampled, vec3(1.75)), 1.0);\n"
"}\n";

uint32_t quadVAO[2] = { 0, 0 };

void draw_lazy_quad(const float width, const float height, const int i)
{
    while (quadVAO[i] == 0)
    {
        uint32_t quadVBO = 0;
        const float quadVertices[] = {

            0.0f,  height, 0.0f, 0.0f, 0.0f,
            0.0f,  0.0f,   0.0f, 0.0f, 1.0f,
            width, height, 0.0f, 1.0f, 0.0f,
            width, 0.0f,   0.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &quadVAO[i]);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO[i]);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 

        break;
    }

    glBindVertexArray(quadVAO[i]);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void graphics_init (Scene * const scene)
{
    gladLoadGL();
    glfwSwapInterval(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    /* Create shaders */
    scene->fbufferShader = shader_init_source (ppu_vs_source, ppu_fs_source);
    scene->debugShader   = shader_init_source (ppu_vs_source, default_fs_source);

    /* Init text and texture objects */
    //text_init ();

    char* pixels = (char*)calloc(192, 1);
    for (int i = 0; i < 192; i += 3)
    {
        uint16_t palColor = palette2C03[i / 3];
        pixels[i] =   (palColor >> 8) << 5;
        pixels[i+1] = (palColor >> 4) << 5;
        pixels[i+2] = (palColor << 5);
    }
    free (pixels);

    /* Create main textures */
    texture_setup (&scene->fbufferTexture, 256, 240, GL_LINEAR, NULL);
    texture_setup (&scene->pTableTexture,  256, 256, GL_NEAREST, NULL);
    texture_setup (&scene->nameTableTexture, 512, 480, GL_NEAREST, NULL);
    texture_setup (&scene->paletteTexture, 64,  1, GL_NEAREST, pixels);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);
}

#ifdef PPU_DEBUG

void draw_ntable_debug (GLFWwindow * window, Scene * const scene)
{
	//glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 215, (GLfloat)scene->bgColor[2] / 184, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);

    /* Render the PPU framebuffer here */
    glUseProgram(scene->debugShader.program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 8, height - 128, 0);
    mat4x4_scale_aniso (model, model, 128, 120, 1.0f);

    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw PPU Nametable textures */
    glBindTexture (GL_TEXTURE_2D, scene->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, NES.ppu.nTableDebug[0]);
	draw_lazy_quad(1.0f, 1.0f, 1);

    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 136, height - 128, 0);
    mat4x4_scale_aniso (model, model, 128, 120, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, scene->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, NES.ppu.nTableDebug[1]);
	draw_lazy_quad(1.0f, 1.0f, 1);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_ptable_debug (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 224, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);

    /* Render the PPU framebuffer here */
    glUseProgram(scene->debugShader.program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 0, 0, 0);
    mat4x4_scale_aniso (model, model, width / 2, height, 1.0f);

    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw PPU Pattern Table textures */
    glBindTexture (GL_TEXTURE_2D, scene->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, NES.ppu.pTableDebug[0]);
	draw_lazy_quad(1.0f, 1.0f, 1);

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 256, 0, 0);
    mat4x4_scale_aniso (model, model, width / 2, height, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(scene->debugShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, scene->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, NES.ppu.pTableDebug[1]);
	draw_lazy_quad(1.0f, 1.0f, 1);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_debug_tiles (int32_t const width, int32_t const height)
{
    /* Draw PPU graphical output */
    //text_begin (width, height);

    char textbuf[256];
    for (int y = 0; y < 30; y++)
    {
        sprintf(textbuf, " ");
        for (int x = 0; x < 32; x++)
        {
            uint8_t tile = ppu_read(&NES.ppu, (y * 32 + x) + 0x2000);
            sprintf(textbuf + strlen(textbuf), "%02x ", tile);
        }
        //text_draw_alpha (textbuf, 0, height - 10 - y * 24, 0.4f, 0xaaffffff);
    }    
}

#endif

void draw_scene (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);

    /* Render the PPU framebuffer here */
    glUseProgram(scene->fbufferShader.program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, width, 0, height, 0, 0.1f);
    mat4x4_identity (model);
    mat4x4_scale_aniso (model, model, width, height, 1.0f);

    glUniformMatrix4fv (glGetUniformLocation(scene->fbufferShader.program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->fbufferShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw framebuffer */
    glBindTexture (GL_TEXTURE_2D, scene->fbufferTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, &NES.ppu.frameBuffer);
	draw_lazy_quad(1.0f, 1.0f, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

#ifdef PPU_DEBUG  
    draw_ntable_debug (window, scene);
#endif
}

void draw_debug (GLFWwindow * window, Timer * const timer)
{
    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);
	uint16_t wOffset = height / 15 * 16;
    wOffset += 4;

#ifdef CPU_DEBUG
    /* Setup text */
    char textbuf[256];     
    text_begin (width, height);
    const GLubyte* vendor = glGetString(GL_VENDOR); // Returns the vendor
    const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model

    /* Vendor and framerate info */
    sprintf(textbuf, "Vendor: %s", vendor);
    text_draw_raised (textbuf, wOffset, height - 16.0f, 0.5f, -1);
    sprintf(textbuf, "Renderer: %s", renderer);
    text_draw_raised (textbuf, wOffset, height - 32.0f, 0.5f, -1);
    sprintf(textbuf, "Frame time: %f ms", timer->frameTime);
    text_draw_raised (textbuf, wOffset, height - 48.0f, 0.5f, -1);

    /* Debug CPU and RAM */
    sprintf(textbuf, "PC: $%04x %02x %s Clk: %ld", cpu->lastpc, cpu->opcode, cpu->lastop, cpu->clockCount);
    text_draw_raised (textbuf, wOffset, height - 64.0f, 0.5f, -1);
    sprintf(textbuf, "Sec: %.3f", ((float)NES.ppu.scanline / 262.0f + NES.ppu.frame) / 60.0f);
    text_draw_raised (textbuf, wOffset, height - 80.0f, 0.5f, -1);
    sprintf(textbuf, "%s", NES.rom.filename);
    text_draw_raised (textbuf, wOffset, height - 160.0f, 0.5f, 0x44ddff);

    /* CPU registers and storage locations for program/vars */
    draw_debug_cpu(wOffset, height - 112.0f);
#endif
}