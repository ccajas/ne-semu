#include <stdio.h>
#include <stdlib.h>
#include "mapper.h"

Mapper mapper_apply (uint8_t header[], uint16_t const mapperID)
{
    Mapper mapper;
    mapper.properties = NULL;

    mapper.PRGbanks = header[4];                       /* Total PRG 16KB banks */
    mapper.CHRbanks = header[5]; /* Total CHR 8KB banks */

    printf("Mapper type %d, read %d PRG bank(s) and %d CHR bank(s). (%d KB and %d KB)\n", mapperID, 
        mapper.PRGbanks, mapper.CHRbanks, 
        mapper.PRGbanks * 16, mapper.CHRbanks * 8);

    if (mapperID == 0) 
    {
        mapper.read  = mapper_NROM_read;
        mapper.write = mapper_NROM_write;
    }
    else if (mapperID == 2) 
    {
        printf("Mapped to 2\n");
        mapper.read  = mapper_UxROM_read;
        mapper.write = mapper_UxROM_write;
    }
    else 
    {
        /* No appropriate mapper could be found, default to 0 (NROM), may have unintended effects */
        mapper.read  = mapper_NROM_read;
        mapper.write = mapper_NROM_write;
    }

    return mapper;
}

/* NROM (mapper 0) */

uint8_t mapper_NROM_read (Mapper * const mapper, uint16_t const address, uint8_t ppu)
{
    if (ppu) 
    {
        return mapper->CHR->data[address];
    }
    /* Choose from 16KB or 32KB in banks to read from. No write possible */
    if (address >= 0x8000 && address <= 0xffff)
    {
        uint32_t mapped_addr = address & (mapper->PRGbanks > 1 ? 0x7fff : 0x3fff);
        return mapper->PRG->data[mapped_addr];
    }
    return 0;
}

void mapper_NROM_write (Mapper * const mapper, uint16_t const address, uint8_t const data, uint8_t ppu)
{
    /* No mapping required. Assumes address is below 0x2000 */
    if (ppu && mapper->CHRbanks == 0) 
    {
        printf("Writing %02x to %04x\n", data, address);
        mapper->CHR->data[address] = data;
        return;
    }
}

/* UnROM (mapper 2) */

uint8_t mapper_UxROM_read (Mapper * mapper, uint16_t const address, uint8_t ppu)
{
    if (ppu)
	{
        printf("Reading from %04x\n", address);
		return mapper_NROM_read (mapper, address, 1);
	}

    if (address < 0x8000) {
        return 0;
    }

    uint32_t mapped_addr = 0;
    mapper->lastBankStart = vc_size(mapper->PRG) - 0x4000;

    if (address >= 0xc000) {
        mapped_addr = vc_size(mapper->PRG) - 0x4000 + (address - 0xc000);
    } else {
        mapped_addr = mapper->bankSelect * 16384 + (address - 0x8000);
    }

    return mapper->PRG->data[mapped_addr];
}

void mapper_UxROM_write (Mapper * const mapper, uint16_t const address, uint8_t const data, uint8_t ppu) 
{
    if (ppu)
	{
        if (address < 0x2000) {
		    mapper_NROM_write (mapper, address, data, 1);
        }
        return;
	}

    if (address < 0x8000) {
        return;
    }

    mapper->bankSelect = data & 0xf;
}