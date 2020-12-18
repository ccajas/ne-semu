#include <stdio.h>
#include <stdint.h>
#include "cpu6502.h"
#include "ppu2c02.h"
#include "rom.h"
#include "utils/filereaders.h"

typedef struct Bus_struct 
{
    uint8_t  ram[64 * 1024];
    CPU6502  cpu;
    PPU2C02  ppu;
    NESrom   rom;
}
Bus;

/* Memory map locations */
extern const uint16_t
    CPU_RAM_START,
    PPU_REG_START,
    APU_IO_START,
    APU_DISABLED,
    PRG_RAM_START,
    PRG_ROM_START,
    PRG_ROM_END;

Bus bus;
extern CPU6502 *cpu;
extern const uint8_t testProg[28];

inline void bus_reset (Bus * const bus)
{
    //bus->ram[0xfffc] = 0x00;
    //bus->ram[0xfffd] = 0xc0;

    ppu_reset (&bus->ppu);
    cpu_reset (&bus->cpu);
    printf("Program counter set to %x \n", bus->cpu.r.pc);
}

/* Run one tick from the entire system */

inline void bus_clock (Bus * const bus)
{
    for (int i = 0; i < 3; i++) 
    {
        ppu_clock (&bus->ppu);
    }
    cpu_clock (&bus->cpu);
}

inline uint8_t cpu_read_mapped (uint16_t const addr, uint32_t * mapped)
{
	// if PRGROM is 16KB
	//     CPU Address Bus          PRG ROM
	//     0x8000 -> 0xBFFF: Map    0x0000 -> 0x3FFF
	//     0xC000 -> 0xFFFF: Mirror 0x0000 -> 0x3FFF
	// if PRGROM is 32KB
	//     CPU Address Bus          PRG ROM
	//     0x8000 -> 0xFFFF: Map    0x0000 -> 0x7FFF
    
    /* Use generic mapper to read 1 bank for now */
	if (addr >= 0x8000 && addr <= 0xffff)
	{
		*mapped = addr & 0x3fff;// (nPRGBanks > 1 ? 0x7fff : 0x3fff);
        return 1;
	}
    return 0;
}

inline uint8_t bus_read (Bus * const bus, uint16_t const address) 
{
    uint8_t  data = 0;
    uint32_t mappedAddress = 0;

    /* Read from system ram 8kB range, fetch 2kB mirror */
    if (address >= 0 && address < 0x2000)
    {
        data = bus->ram[address & 0x07ff];
        //printf("Read from system RAM: %04x\n", addr);
    }
	/* Read from PPU, mirrored every 8 bytes */
    else if (address >= 0x2000 && address < 0x4000)
	{
        printf("Attempting to read from PPU at register %x, pc:%04x\n", address & 0x7, bus->cpu.lastpc);
		ppu_cpu_read (&bus->ppu, address & 0x7);
	}
    /* Read from APU and I/O */
    else if (address >= 0x4000 && address < 0x4020)
    {

    }
    /* Read from cartridge space */
    else if (address >= 0x4020 && address <= 0xffff)
    {
        if (bus->rom.valid && cpu_read_mapped(address, &mappedAddress))
            data = bus->rom.prg_data[mappedAddress];
    }
    
    return data;
}

inline void bus_write (Bus * const bus, uint16_t const addr, uint8_t const data) 
{
    /* Read from system ram 8kB range, write to 2kB mirror */
    //printf("Data write at addr: %04x\n", addr);
	if (addr >= 0 && addr < 0x2000)
    {
		bus->ram[addr & 0x7ff] = data;
    }
	else if (addr >= 0x2000 && addr <= 0x3fff)
	{
		/* PPU Address range, mirrored every 8 */
		//ppu_write (addr & 0x7, data);
        printf("Attempting to write to PPU at register %x, pc:%04x\n", addr & 0x7, bus->cpu.lastpc);
	}
}

/* Dummy function */

inline void rom_load (Bus * const bus, const char* filename)
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
    /* Test disassembly output */
    cpu_disassemble(bus, 0xc000, 0xc200);

    printf("(%s)\n", filename);
    free (filebuf);
}