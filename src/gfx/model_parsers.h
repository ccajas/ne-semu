#ifndef MODEL_PARSERS_H
#define MODEL_PARSERS_H

/* Forward declaration */

typedef struct Model_struct Model;
void model_load (Model* const model, const char* filename);

/* Concrete model loading functions */

void model_read_collada (Model* const model);
void model_read_FBX_ascii (Model* const model);
void model_read_FBX_bin (Model* const model);
void model_read_glTF_ascii (Model* const model);
void model_read_OBJ (Model* const model);
void model_read_PLY_ascii (Model* const model);
void model_read_PLY_bin (Model* const model);
void model_read_STL_ascii (Model* const model);
void model_read_STL_bin (Model* const model);

/* Companion file reader for OBJ material files */

void model_read_OBJ_MTL (Model* const model, char* fileSrc);

/* Catch-all functions to load either binary or ASCII variants.
   This does a simple check for header content to determine the type. */

void model_read_FBX_bin_or_ascii (Model* const model);
void model_read_PLY_bin_or_ascii (Model* const model);
void model_read_STL_bin_or_ascii (Model* const model);

#endif