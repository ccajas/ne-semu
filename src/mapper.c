#include <stdio.h>
#include <stdlib.h>
#include "mapper.h"

Mapper mapper_apply (Mapper * const mapper, uint8_t header[], uint16_t const mapperID)
{
    Mapper mapper1;
    mapper1.properties = NULL;

    mapper1.PRGbanks = header[4]; /* Total PRG 16KB banks */
    mapper1.CHRbanks = header[5]; /* Total CHR 8KB banks */

    if (mapperID == 0) 
    {
        mapper1.cpuReadWrite = mapper_NROM_cpu_rw;
        mapper1.ppuReadWrite = mapper_NROM_ppu_rw;
    }
    else /* No appropriate mapper could be found, default to 0 (NROM), will have unintended effects */
    {
        mapper1.cpuReadWrite = mapper_NROM_cpu_rw;
    }

    return mapper1;
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

uint32_t mapper_NROM_cpu_rw (Mapper * const mapper, uint16_t const address)
{    
    /* Choose from 16KB or 32KB in banks to read from. No write possible */
    return address & ((mapper->PRGbanks > 1) ? 0x7fff : 0x3fff);
}

void mapper_NROM_ppu_rw (uint16_t const address, uint32_t * mapped)
{
    /* No mapping required. Assumes address is below 0x2000 */
	*mapped = address;
}