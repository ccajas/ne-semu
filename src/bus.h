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

Bus NES;
extern CPU6502 *cpu;

inline void bus_reset (Bus * const bus)
{
    bus->clockCount = 0;

    ppu_reset (&bus->ppu, &bus->rom);
    cpu_reset (&bus->cpu);
    printf("Program counter set to %x \n", bus->cpu.r.pc);
}

/* Run one tick from the entire system */

inline void bus_clock (Bus * const bus)
{
    bus->clockCount++;
    if (bus->ppu.clockCount % 3 == 0) 
    {
        cpu_clock (bus);
    }
    ppu_clock (&bus->ppu);
}

inline void bus_cpu_step (Bus * const bus)
{
    bus->cpu.clockGoal++;
    for (int i = 0; i < 3; i++)
    {
        bus_clock (bus);
    }
}

inline void bus_exec (Bus * const bus, uint32_t const tickcount)
{
    bus->cpu.clockGoal += tickcount;

    while (bus->cpu.clockCount < bus->cpu.clockGoal) 
        bus_clock (bus);
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
        //printf("Attempting to read from PPU at register %x, pc:%04x data:%02x\n", address & 0x7, bus->cpu.lastpc, data);
		data = ppu_register_read (&bus->ppu, address & 0x7);
	}
    /* Read from APU and I/O */
    else if (address >= 0x4000 && address < 0x4016)
    {

    }
    /* Read out controller status(es) starting from the top bit */
	else if (address == 0x4016 || address == 0x4017)
	{
        if (bus->controllerState[address & 1] & 0x80) {
            printf("Read controller: %02x\n", bus->controllerState[address & 1]);
        }

        data = (bus->controllerState[address & 1] & 0x80) > 0;
        bus->controllerState[address & 1] <<= 1;
    }
    /* Read from cartridge space */
    else if (address >= 0x4020 && address <= 0xffff)
    {
        if (bus->rom.mapper.cpuReadWrite)
        {
            uint32_t mappedAddress = bus->rom.mapper.cpuReadWrite(&bus->rom.mapper, address);
            data = vc_get (&bus->rom.PRGdata, mappedAddress);
        }
    }
    
    return data;
}

inline void bus_write (Bus * const bus, uint16_t const address, uint8_t const data) 
{
    /* Read from system ram 8kB range, write to 2kB mirror */
    //printf("Data write at addr: %04x\n", addr);
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
    else if (address == 0x4016 || address == 0x4017)
    {
        bus->controllerState[address & 1] = bus->controller[address & 1];
    }
}