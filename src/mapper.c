#include "mapper.h"

void mapper_implement (Mapper * const mapper, uint16_t const mapperID)
{
    mapper->PRGbanks = 1;
    mapper->CHRbanks = 1;

    if (mapperID == 0) 
    {
        mapper->cpuReadWrite = mapper_0_cpu_rw;
        //mapper->ppuReadWrite = mapper_0_ppu_rw;
    }
    /*if (!strcmp(filename, "default")) {
        return;
    }
    if (filename != NULL) {
        strcpy(model->pathName, pathname (filename));
    }

    model_free (model);
    model_init (model);
    model->fileBuf = read_file_short (filename);

    if (model->fileBuf) 
    {
	    char modelType[20] = "model_";
	    strcat(modelType, (char*) GET_FILE_EXT (filename));

        for (const NameFunction* ptr = modelParsers; strcmp(ptr->name, "\0"); ptr++)
        {
            if(!strcmp(ptr->name, modelType)) {
                ptr->parserFunc(model);
            }
        }
        free (model->fileBuf);
        
        if (model->meshCount == 0) {
            model_create_mesh (model, "DefaultMesh", 0);
        }
        if (model->materialCount == 0) {
            model_create_material (model, "DefaultMaterial");
        }
        model_load_textures (model);
    }
    else {
        return;
    }*/
}

uint32_t mapper_0_cpu_rw (Mapper * const mapper, uint16_t const address)
{    
    /* Choose from 16KB or 32KB in banks to read from */
    return address & ((mapper->PRGbanks > 1) ? 0x7fff : 0x3fff);
}