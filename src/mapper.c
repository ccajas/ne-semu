#include <stdio.h>
#include <stdlib.h>
#include "mapper.h"

Mapper mapper_apply (uint8_t header[], uint16_t const mapperID)
{
    Mapper mapper;
    mapper.properties = NULL;

    mapper.PRGbanks = header[4]; /* Total PRG 16KB banks */
    mapper.CHRbanks = header[5]; /* Total CHR 8KB banks */

    printf("Read %d PRG bank(s) and %d CHR bank(s). (%d KB and %d KB)\n", mapper.PRGbanks, mapper.CHRbanks, 
        mapper.PRGbanks * 16, mapper.CHRbanks * 8);

    if (mapperID == 0) 
    {
        mapper.cpuReadWrite = mapper_NROM_cpu_rw;
        mapper.ppuReadWrite = mapper_NROM_ppu_rw;
    }
    else
    {
        /* No appropriate mapper could be found, default to 0 (NROM), may have unintended effects */
        mapper.cpuReadWrite = mapper_NROM_cpu_rw;
    }

    return mapper;
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