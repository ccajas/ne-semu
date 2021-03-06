#ifndef BUS_H
#define BUS_H

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

    /* Controllers */
    uint8_t controller[2];
    uint8_t controllerState[2];

    /* Master clock */
    uint64_t clockCount;
}
Bus;

extern Bus NES;
extern CPU6502 *cpu;

inline void bus_reset (Bus * const bus)
{
    bus->clockCount = 0;

    ppu_reset (&bus->ppu, &bus->rom);
    cpu_reset (&bus->cpu);
    printf("Program counter set to %x \n", bus->cpu.r.pc);
}

inline void bus_exec (Bus * const bus, uint32_t const tickcount)
{
    bus->cpu.clockGoal += tickcount;
    bus->ppu.clockGoal += tickcount * 3;

    uint32_t ticks = tickcount;

    while (--ticks)
    {
        ++bus->clockCount;

        ppu_clock (&bus->ppu);
        ppu_clock (&bus->ppu);
        ppu_clock (&bus->ppu);

        cpu_clock (bus);
    }
}

/* Run one instruction from the CPU */

inline void bus_cpu_tick (Bus * const bus)
{
    bus_exec (bus, bus->cpu.clockticks + 1);
}

/* Run one scanline */

inline void bus_scanline_step (Bus * const bus)
{
    bus->ppu.clockGoal += 341;

    while (bus->ppu.clockCount < bus->ppu.clockGoal)
    {
        if (bus->clockCount++ % 3 == 0)
            cpu_clock (bus);

        ppu_clock (&bus->ppu);
    }
}

inline uint8_t bus_read (Bus * const bus, uint16_t const address) 
{
    uint8_t data = 0;

    /* Read from system ram 8kB range, fetch 2kB mirror */
    if (address >= 0 && address < 0x2000)
    {
        data = bus->ram[address & 0x07ff];
        /* printf("Read from system RAM: %04x\n", addr); */
    }
	/* Read from PPU, mirrored every 8 bytes */
    else if (address >= 0x2000 && address < 0x4000)
	{
        /* printf("Attempting to read from PPU at register %x, pc:%04x data:%02x\n", address & 0x7, bus->cpu.lastpc, data); */
		data = ppu_register_read (&bus->ppu, address & 0x7);
	}
    /* Read from APU and I/O */
    else if (address >= 0x4000 && address < 0x4016)
    {

    }
    /* Read out controller status(es) starting from the top bit */
	else if (address == 0x4016 || address == 0x4017)
	{
        data = (bus->controllerState[address & 1] & 0x80) > 0;
        bus->controllerState[address & 1] <<= 1;
    }
    /* Read from cartridge space */
    else if (address >= 0x4020 && address <= 0xffff)
    {
        if (bus->rom.mapper.read)
        {
            data = bus->rom.mapper.read (&bus->rom.mapper, address, 0);
        }
    }
    
    return data;
}

inline void bus_write (Bus * const bus, uint16_t const address, uint8_t const data) 
{
    /* Read from system ram 8kB range, write to 2kB mirror */
	if (address >= 0 && address < 0x2000)
    {
		bus->ram[address & 0x7ff] = data;
    }
	/* PPU Address range, mirrored every 8 */
	else if (address >= 0x2000 && address <= 0x3fff)
	{
        //printf("Attempting to write to PPU at register %x, pc:%04x data:%02x\n", address & 0x7, bus->cpu.lastpc, data);
		ppu_register_write (&bus->ppu, address & 0x7, data);
	}
    /* Write to OAM DMA register */
    else if (address == 0x4014)
    { 
        uint16_t DMApage = (uint16_t)data << 8;

        for (int i = 0; i < 256; i++) 
        {
            ppu_oam_dma_write (&bus->ppu, bus_read (bus, DMApage + i));
        }
    }
    else if (address == 0x4016 || address == 0x4017)
    {
        bus->controllerState[address & 1] = bus->controller[address & 1];
    }
    /* Write to cartridge */
    else if (address >= 0x8000 && address <= 0xffff)
    {
        if (bus->rom.mapper.write)
        {
            bus->rom.mapper.write (&bus->rom.mapper, address, data, 0);
        }
    }
}

#endif
