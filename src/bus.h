#include <stdio.h>
#include <stdint.h>
#include "cpu6502.h"
#include "ppu2c02.h"
#include "rom.h"
#include "utils/filereaders.h"

typedef struct Bus_struct 
{
    /* Bus components */
    uint8_t  ram[2 * 1024];
    CPU6502  cpu;
    PPU2C02  ppu;
    NESrom   rom;

    /* Clock and helper vars */
    uint64_t clockCount;
}
Bus;

Bus NES;
extern CPU6502 *cpu;

inline void bus_reset (Bus * const bus)
{
    bus->clockCount = 0;

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

inline void bus_exec (Bus * const bus, uint32_t const tickcount)
{
    bus->clockCount++;
    bus->cpu.clockGoal += tickcount;

    while (bus->cpu.clockCount < bus->cpu.clockGoal) 
    {
        bus_clock (bus);
    }
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
    uint8_t data = 0;

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
        if (bus->rom.mapper.cpuReadWrite)
        {
            uint32_t mappedAddress = bus->rom.mapper.cpuReadWrite(&bus->rom.mapper, address);
            data = bus->rom.prg_data[mappedAddress];
        }
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