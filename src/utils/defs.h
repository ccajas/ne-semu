#ifndef _DEFS_H
#define _DEFS_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define e_printf(format, ...) printf ("\033[1;31m%s\033[0m", __VA_ARGS__)

// Small helper functions and structures

static inline const char* GET_FILE_EXT(const char *filename) 
{
   const char *ext = strrchr(filename, '.');
   return (ext && ext != filename) ? ext + 1 : (filename + strlen(filename));
}

inline int is_empty(const char* string)
{
   return (string[0] == '\0');
   //return (!strcmp(string, " ") || !strcmp(string, ""));
}

inline uint32_t from_fourCC (const char* value) 
{
   return (uint32_t)(value[0] +
      (value[1] << 8) +
      (value[2] << 16) +
      (value[3] << 24)
   );
}

inline char* to_fourCC (const uint32_t value) 
{
   char* out = malloc(sizeof(char) * 4);
   char fourcc[4] = {
      (char)(value & 255),
      (char)((value >> 8) & 255),
      (char)((value >> 16) & 255),
      (char)((value >> 24) & 255)
   };
   strcpy(out, fourcc);
   return out;
}

#endif