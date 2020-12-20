
#include "bus.h"

void rom_eject (NESrom * const rom)
{
    vc_free (&rom->PRGdata);
    vc_free (&rom->CHRdata);

    rom->valid = 0;
}

void rom_load (Bus * const bus, const char* filename)
{
    NESrom * rom = &bus->rom;
    rom->valid = 0;

    uint8_t *filebuf = (uint8_t*)read_file_short (filename);
    uint32_t headerString = 0;
    memcpy (&headerString, filebuf, sizeof(headerString));

    if (headerString == 0x1a53454e) /* Chars "NES" + 0x1a */
    {
        printf("Rom valid!\n");
        printf("(%s)\n", filename);
        rom_eject (&bus->rom);

        rom->valid = 1;
        uint8_t mapperID = (rom->header[6] >> 4) | (rom->header[7] & 0xf0);

        /* Copy header and PRG rom from 16 byte offset */
        memcpy (rom->header, filebuf, sizeof(rom->header));

        /* After getting the rom info, the correct mapper can be obtained */
        rom->mirroring = rom->header[6] & 1;
        rom->mapper    = mapper_apply (&rom->mapper, rom->header, mapperID); // Default is 0
        printf("Reading...");

        /* Add trainer data if needed */


        /* Add the PRG and CHR data */
        vc_init (&rom->PRGdata, 1);
        vc_init (&rom->CHRdata, 1);

        vc_push_array (&rom->PRGdata, filebuf, rom->mapper.PRGbanks * 16384, sizeof(rom->header));
        vc_push_array (&rom->CHRdata, filebuf, rom->mapper.CHRbanks * 8192,  sizeof(rom->header) + 
            rom->PRGdata.capacity);

        /* Test disassembly output and CHR data dump */
        cpu_disassemble (bus, 0xc000, 0xc200);

        dump_pattern_table (&bus->ppu, &bus->rom, 0);
        dump_pattern_table (&bus->ppu, &bus->rom, 1);
    }

    free (filebuf);
}