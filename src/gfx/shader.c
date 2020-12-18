#include <assert.h>
#include <stdio.h>
#include "gl_gen.h"
#include "shader.h"

const char* defaultVertSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 model;"
    "uniform mat4 view;"
    "uniform mat4 projection;"
    "out vec3 vPos;"
    "void main()\n"
    "{\n"
    "   vPos = aPos;\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\0";

const char* defaultFragSource = "#version 330 core\n"
    "in vec3 vPos;"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(vPos * 0.5f + 0.5f, 1.0f);\n"
    "}\n\0";

Shader shader_init (const char *filenameVS, const char *filenameFS)
{
    assert (filenameVS && filenameFS);

    int vertexShader = 0, fragmentShader = 0;
    Shader shader = {0};
    shader.program = 0;

    if (!is_empty(filenameVS) && !is_empty(filenameFS))
    {
        const char* vertSource = read_file_short(filenameVS);
	    const char* fragSource = read_file_short(filenameFS); 

        vertexShader   = shader_build(vertSource, GL_VERTEX_SHADER,   shader.program);
        fragmentShader = shader_build(fragSource, GL_FRAGMENT_SHADER, shader.program);      
    }
    else {
        vertexShader   = shader_build(defaultVertSource, GL_VERTEX_SHADER,   shader.program);
        fragmentShader = shader_build(defaultFragSource, GL_FRAGMENT_SHADER, shader.program);      
    }

    shader.program = shader_link(vertexShader, fragmentShader);
    return shader;
}

Shader shader_init_source (const char *codeVS, const char *codeFS)
{
    assert (codeVS && codeFS);

    int vertexShader = 0, fragmentShader = 0;
    Shader shader = {0};
    shader.program = 0;

    if (!is_empty(codeVS) && !is_empty(codeFS))
    {
        vertexShader   = shader_build(codeVS, GL_VERTEX_SHADER,   shader.program);
        fragmentShader = shader_build(codeFS, GL_FRAGMENT_SHADER, shader.program);      
    }
    else {
        vertexShader   = shader_build(defaultVertSource, GL_VERTEX_SHADER,   shader.program);
        fragmentShader = shader_build(defaultFragSource, GL_FRAGMENT_SHADER, shader.program);      
    }

    shader.program = shader_link(vertexShader, fragmentShader);
    return shader;
}

uint32_t shader_link (const int32_t vertexShader, const int32_t fragmentShader)
{
    // link shaders
    int success;
    char infoLog[1024];
    uint32_t shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR: SHADER PROGRAM LINKING_FAILED \n%s \n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void shader_apply (Shader const *shader, const char *uniformName, const void * param)
{
    int32_t location = shader_get_location (shader, uniformName);
    glUniformMatrix4fv (location, 1, GL_FALSE, (GLfloat*) param);
}

void shader_apply_1f (Shader const *shader, const char *uniformName, const float value)
{
    int32_t location = shader_get_location (shader, uniformName);
    glUniform1f (location, value);
}

void shader_apply_3f (Shader const *shader, const char *uniformName, const void * param)
{
    int32_t location = shader_get_location (shader, uniformName);
    glUniform3fv (location, 1, (GLfloat*) param);
}

void shader_apply_int (Shader const *shader, const char *uniformName, const uint32_t value)
{
    int32_t location = shader_get_location (shader, uniformName);
    glUniform1i (location, value);
}