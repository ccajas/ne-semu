
#include "bus.h"

void rom_load (Bus * const bus, const char* filename)
{
    NESrom * rom = &bus->rom;
    rom->valid = 0;

    uint8_t *filebuf = (uint8_t*)read_file_short (filename);
    uint32_t fptr = sizeof(rom->header);

    /* Copy header and PRG rom from 16 byte offset */
    memcpy (rom->header,   filebuf,        sizeof(rom->header));
    memcpy (rom->name,     filebuf,        sizeof(rom->name));
    memcpy (rom->prg_data, filebuf + fptr, sizeof(rom->prg_data));
    
    fptr += sizeof(rom->prg_data);
    memcpy (rom->chr_data, filebuf + fptr, sizeof(rom->chr_data));

    if (rom->name[0] == 0x4e && rom->name[1] == 0x45 && rom->name[2] == 0x53 && rom->name[3] == 0x1a)
    {
        printf("Rom valid!\n");
        rom->valid = 1;
    }
    /* After getting the rom info, the correct mapper can be obtained */
    mapper_implement(&rom->mapper, 0); // Default is 0

    /* Test disassembly output */
    cpu_disassemble (bus, 0xc000, 0xc200);

    /* Test CHR dump */
    dump_pattern_table (&bus->ppu, &bus->rom, 0);
    dump_pattern_table (&bus->ppu, &bus->rom, 1);

    printf("(%s)\n", filename);
    free (filebuf);
}