#ifndef SHADER_H
#define SHADER_H

#include "../utils/defs.h"
#include "../utils/filereaders.h"

typedef struct IndexNamePair_struct
{
    uint32_t typeIndex;
    char     name[256];
}
IndexNamePair;

typedef struct Shader_struct
{
    int32_t program;
    int32_t *locations;
}
Shader;

void shader_apply     (Shader const *shader, const char *uniformName, const void * param);
void shader_apply_1f  (Shader const *shader, const char *uniformName, const float value);
void shader_apply_3f  (Shader const *shader, const char *uniformName, const void * param);
void shader_apply_int (Shader const *shader, const char *uniformName, const uint32_t value);

Shader shader_init (const char *filenameVS, const char *filenameFS);
Shader shader_init_source (const char *codeVS, const char *codeFS);
uint32_t shader_link (const int32_t vertexShader, const int32_t fragmentShader);

void shader_apply_direct (Shader const *shader, uint32_t location, void *value);


inline GLuint shader_build (const GLchar *programSrc, const GLenum type, GLuint program)
{
	const GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &programSrc, NULL);
	glCompileShader(shader);
	GLint isCompiled = 0;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		e_printf("%s", "Shader error!\n");

		/* The maxLength includes the NULL character */
		GLchar* errorLog = malloc(sizeof(char) * maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		e_printf("%s", errorLog);
		glDeleteShader(shader);

		/* Exit with failure */
		return -1;
	}
	glAttachShader(program, shader);
	return shader;
}

inline int32_t shader_get_location (Shader const *shader, const char *uniformName)
{
    return glGetUniformLocation(shader->program, uniformName);
}

#endif