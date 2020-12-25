#include <stdio.h>
#include <stdint.h>

typedef struct CPU6502_struct
{
    /* 6502 CPU registers */
	struct Registers {
        uint16_t pc;
        uint8_t  sp;
        uint8_t  status;
        uint8_t  a, x, y;
	} r;

    /* Helper vars */
    uint64_t instructions, clockCount, clockGoal;
    uint16_t lastpc, abs_addr, rel_addr, value, result;
    uint8_t  opcode;
    uint8_t  clockticks;

    /* Meta vars */
    uint8_t  debug;
    uint8_t  opID;
    char     lastop[8], lastmode[8];
}
CPU6502;

/* Instrction/mode/clock tick group */
struct Instruction
{		
    void    (*op)();
    uint8_t (*addrmode)();
    uint8_t ticks;
};

/* Forward declaration */
typedef struct Bus_struct     Bus;

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

void cpu_reset       (CPU6502 * const cpu);
void cpu_clock       (Bus     * const bus);
void cpu_exec        (CPU6502 * const cpu, uint32_t const tickcount);
void cpu_disassemble (Bus     * const bus, uint16_t const start, uint16_t const end);
void nmi();