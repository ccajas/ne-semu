#include "graphics.h"

const char * ppu_vs_source =

"#version 330\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
"layout (location = 1) in vec2 texture;"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"

"void main()\n"
"{\n"
"    vec4 localPos = model * vec4(vertex.xyz, 1.0);"
"    gl_Position = projection * vec4(localPos.xy, 0.0, 1.0);\n"
"    TexCoords = texture.xy;\n"
"}\n";
 
const char * ppu_fs_source =

"#version 130\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"

"vec3 applyVignette(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy) - vec2(0.5);\n"           
"    float dist = length(position);\n"

"    float radius = 1.35;\n"
"    float softness = 1.0;\n"
"    float vignette = smoothstep(radius, radius - softness, dist);\n"
"    color.rgb = color.rgb - (0.92 - vignette);\n"
"    return color;\n"
"}\n"

"float mod(float x, float y) { return x - (y * floor( x / y )); }\n"

"vec3 applyScanline(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    float px = 1.0/512.0;\n"
"    position.x += mod(position.y * 240.0, 2.0) * px;\n"
"    color *= pow(fract(position.x * 256.0), 0.2);\n"
"    color *= pow(fract(position.y * 240.0), 0.5);\n"
"    return color * 1.75;\n"
"}\n"

"vec3 blur5(sampler2D image, vec2 uv, vec2 resolution, vec2 direction)\n"
"{\n"
"    vec3 color = vec3(0.0);\n"
"    vec2 off1 = vec2(1.3333333333333333) * direction;\n"
"    color += texture2D(image, uv).rgb * 0.29411764705882354;\n"
"    color += texture2D(image, uv + (off1 / resolution)).rgb * 0.35294117647058826;\n"
"    color += texture2D(image, uv - (off1 / resolution)).rgb * 0.35294117647058826;\n"
"    return color;\n"
"}"

"void main()\n"
"{\n"
"    vec3 tint    = vec3(113, 115, 126) / 127.0;\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    sampled      = pow(sampled, vec3(1/1.4));\n"
"    sampled      = applyScanline(applyVignette(sampled));\n"
"    gl_FragColor = vec4(sampled * sampled, 1.0);\n"
"}\n";

uint32_t quadVAO = 0;
uint32_t quadVBO = 0;

const float quadVertices[] = {

     0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
     1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
     1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
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

#ifdef PPU_DEBUG

void ppu_debug (Scene * const scene, int32_t const scrWidth, int32_t const scrHeight)
{
    mat4x4 model;
    uint16_t w = scrHeight / 15 * 16;

    /* Draw pattern tables */
    mat4x4_identity (model);
    mat4x4_translate_in_place (model, w, 0, 0);
    mat4x4_scale_aniso (model, model, 256, 256, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(scene->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, scene->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, &NES.ppu.pTableDebug[0]);
	//draw_lazy_quad();

    mat4x4_identity (model);
    mat4x4_translate_in_place (model, w + 256, 0, 0);
    mat4x4_scale_aniso (model, model, 256, 256, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(scene->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, scene->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, &NES.ppu.pTableDebug[1]);

	//draw_lazy_quad();

	/* Bind the palette to the 2nd texture too */
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, scene->paletteTexture);

	/* Finish */
    glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_debug_ppu (int32_t const width, int32_t const height)
{
    /* Draw PPU graphical output */
    text_begin (width, height);

    char textbuf[256];
    for (int y = 0; y < 30; y++)
    {
        sprintf(textbuf, " ");
        for (int x = 0; x < 32; x++)
        {
            uint8_t tile = ppu_read(&NES.ppu, (y * 32 + x) + 0x2000);
            sprintf(textbuf + strlen(textbuf), "%02x ", tile);
        }
        text_draw_alpha (textbuf, 0, height - 10 - y * 24, 0.4f, 0xaaffffff);
    }    
}

#endif

void draw_scene (GLFWwindow * window, Scene * const scene)
{
	glClearColor((GLfloat)scene->bgColor[0] / 255, (GLfloat)scene->bgColor[1] / 255, (GLfloat)scene->bgColor[2] / 255, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);
	uint16_t w = height / 15 * 16;

    /* Render the PPU framebuffer here */
    glUseProgram(scene->fbufferShader.program);
    mat4x4 p;
    mat4x4 model;

    mat4x4_identity (model);
    mat4x4_scale_aniso (model, model, w, height, 1.0f);
    mat4x4_ortho (p, 0, width, 0, height, 0, 0.1f);

    glUniformMatrix4fv (glGetUniformLocation(scene->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->fbufferShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) p);

    glActiveTexture (GL_TEXTURE0);

    /* Draw framebuffer */
    glBindTexture (GL_TEXTURE_2D, scene->fbufferTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, &NES.ppu.frameBuffer);
	draw_lazy_quad();

    //ppu_debug (scene, width, height);
}

void draw_debug (GLFWwindow * window, Timer * const timer)
{
    update_timer (timer, glfwGetTime());

    int32_t width, height;
    glfwGetFramebufferSize (window, &width, &height);
	uint16_t wOffset = height / 15 * 16;
    wOffset += 4;

#if PPU_DEBUG
    /* Graphical PPU debug */
    if (ppu_show_debug (&NES.ppu))
        draw_debug_ppu (width, height);
#endif

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
}