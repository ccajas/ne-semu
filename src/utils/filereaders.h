#ifndef FILEREADERS_H
#define FILEREADERS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

inline char* pathname (const char* filename)
{
    int index = 0;

    for(int i = strlen(filename); i >= 0; i--) 
    {
        if(filename[i] == '/') {
            index = i;
            break;
        }
    }
    char* dir = malloc((index + 1) * sizeof(char));
    strncpy(dir, filename, index);
    dir[index] = '\0';
    strcat(dir, "/");
    
    return dir;
}

inline char* read_file_short (const char* filename)
{	
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
        printf("Cannot open file '%s'\n", filename);
        return NULL;
    }

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	printf("Bytes read: %lu (\033[96m'%s'\033[0m)\n", size, filename);

	char *buf = malloc(size + 1);
	if (!fread(buf, 1, size, f)) 
	{
		fclose(f);
		return NULL;
	}
	fclose(f);

	/* End with null character */
	buf[size] = 0;

	return buf;
}

#endif