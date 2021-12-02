#include <libgen.h>
#include "bus.h"

void rom_eject (NESrom * const rom)
{
    vc_free (&rom->PRGdata);
    vc_free (&rom->CHRdata);
    vc_free (&rom->mapper.localCHR);

    memset(&rom->filename[0], 0, sizeof(rom->filename));
    rom->valid = 0;
    rom->mapper.usesCHR = 0;
}

uint8_t rom_load (Bus * const bus, const char* pathname)
{
    NESrom * rom = &bus->rom;
    rom->valid = 0;

    uint8_t *filebuf = (uint8_t*)read_file_short (pathname);
    uint32_t headerString = 0;
    memcpy (&headerString, filebuf, sizeof(headerString));

    if (headerString == 0x1a53454e) /* Chars "NES" + 0x1a */
    {
        const char * file = basename(pathname);
        rom_eject (&bus->rom);

        rom->valid = 1;

        /* Copy header and PRG rom from 16 byte offset */
        memcpy (rom->header,   filebuf, sizeof(rom->header));
        memcpy (rom->filename, file,    128);
        rom->mapperID = (rom->header[6] >> 4) | (rom->header[7] & 0xf0);
        printf("Rom valid! (%s)\n", rom->filename);

        /* After getting the rom info, the correct mapper can be obtained */
        rom->mirroring = rom->header[6] & 1;
        rom->mapper    = mapper_apply (rom->header, rom->mapperID);

        /* Add the PRG and CHR data, pre-allocate 8KB if CHR RAM */
        vc_init (&rom->PRGdata, 1);
        vc_init (&rom->CHRdata, 1);
        vc_init (&rom->mapper.localCHR, 1);

        printf("Mirroring: %s\n", rom->mirroring == 0 ? "Horizontal" : "Vertical");
        /* Add trainer data if needed */

        vc_push_array (&rom->PRGdata, filebuf, rom->mapper.PRGbanks * 16384, sizeof(rom->header));
        vc_push_array (&rom->CHRdata, filebuf, rom->mapper.CHRbanks * 8192,  sizeof(rom->header) + rom->PRGdata.total);

        if (rom->mapper.CHRbanks == 0) {
            //for (int i = 0; i < 0x4000; i++) vc_push (&rom->CHRdata, 0);
        }

        /* Uses local CHR */
        if (vc_size(&rom->CHRdata) == 0)
        {
            for (int i = 0; i < 0x4000; i++) vc_push (&rom->mapper.localCHR, 0);
            rom->mapper.usesCHR = 1;
        }

        rom->mapper.PRG = &rom->PRGdata;
        rom->mapper.CHR = &rom->CHRdata;
        rom->mapper.lastBankStart = vc_size(&rom->PRGdata) - 0x4000;

        bus_reset (bus);
        printf("capacity: %d %d \n", vc_size(&rom->PRGdata), vc_size(&rom->CHRdata));

        /* Test disassembly output */
        //cpu_disassemble (bus, bus->cpu.r.pc, bus->cpu.r.pc + 0x80);
        return 1;
    }

    free (filebuf);
    return 0;
}