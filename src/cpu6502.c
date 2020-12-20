/* Fake6502 CPU emulator core v1.1 *******************
 * (c)2011-2013 Mike Chambers                        *
 *****************************************************/

#include <string.h>
#include "bus.h"

//externally supplied functions
uint8_t cpu_read (uint16_t address)                { return bus_read(&NES, address);  }
void    cpu_write(uint16_t address, uint8_t value) { bus_write(&NES, address, value); }

//6502 defines
CPU6502 *cpu = &NES.cpu;

#define saveaccum(n) cpu->r.a = (uint8_t)((n) & 0xff)

/* flag modifier macros */
#define flag_set(f)   cpu->r.status |= f
#define flag_clear(f) cpu->r.status &= (~f)

/* Meta function macros (for disassembly/debugging) */

#define get_opname()   if (cpu->debug) { sprintf(cpu->lastop,   "%s",  __func__); return; } 
#define get_addrmode() if (cpu->debug) { sprintf(cpu->lastmode, "%4s", __func__); return; } 
#define to_upper(str) {\
    char *s = str;\
    while (*s) {*s = (unsigned char)(*s - 32); s++; }\
}

//flag calculation macros
#define zerocalc(n) {\
    if ((n & 0xff) == 0) flag_set(FLAG_ZERO);\
        else flag_clear(FLAG_ZERO);\
}

#define signcalc(n) {\
    if ((n) & 0x80) flag_set(FLAG_SIGN);\
        else flag_clear(FLAG_SIGN);\
}

#define carrycalc(n) {\
    if ((n) & 0xff00) flag_set(FLAG_CARRY);\
        else flag_clear(FLAG_CARRY);\
}

    /*if (~((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x80) setoverflow();\*/
#define overflowcalc(n, m, o) { /* n = cpu->result, m = accumulator, o = memory */ \
    uint8_t of = (~((uint16_t)(m) ^ (uint16_t) (o)) & ((uint16_t)(m) ^ (uint16_t)(n)));\
    if (of & 0x80) flag_set(FLAG_OVERFLOW);\
        else flag_clear(FLAG_OVERFLOW);\
}

/* Branch macro */
#define branch() { \
    cpu->clockticks++;\
    cpu->abs_addr = cpu->r.pc + cpu->rel_addr;\
    if ((cpu->abs_addr & 0xff00) != (cpu->r.pc & 0xff00))\
        cpu->clockticks++;\
    cpu->r.pc = cpu->abs_addr;\
}

/* Forward declare the op table */

struct  Instruction optable[256];
uint8_t penaltyop, penaltyaddr;

void cpu_reset (CPU6502 * const cpu) 
{
    cpu->abs_addr = 0xfffc;

	/* Reset/clear registers */
    cpu->r.pc = (uint16_t)cpu_read(cpu->abs_addr) | ((uint16_t)cpu_read(cpu->abs_addr + 1) << 8);
    cpu->r.a = 0;
    cpu->r.x = 0;
    cpu->r.y = 0;
    cpu->r.sp = 0xfd;
    cpu->r.status = 0 | FLAG_CONSTANT | FLAG_INTERRUPT;

	/* Reset helper vars */
	cpu->abs_addr = 0x0;
	cpu->rel_addr = 0x0;
	cpu->value = 0;
    cpu->debug = 0;

	/* Takes 7 cycles to reset */
    cpu->clockticks = 7;
}

void cpu_clock (CPU6502 * const cpu)
{
    if (cpu->clockticks == 0)
    {
        cpu->lastpc = cpu->r.pc;
        cpu->opcode = cpu_read(cpu->r.pc++);
        cpu->r.status |= FLAG_CONSTANT;

        /* Fetch OP name, convert op position from table into ID */
        cpu->debug = 1;
        cpu->opID = ((cpu->opcode & 3) * 0x40) + (cpu->opcode >> 2);

        (*optable[cpu->opID].addrmode)();
        (*optable[cpu->opID].op)();

        to_upper(cpu->lastop);
        cpu->debug = 0;

        printf("$%04x %02x %s (%s) : A:%02x X:%02x Y:%02x P:%02x SP:%02x CYC:%ld\n", 
            cpu->lastpc, cpu->opcode, cpu->lastop, cpu->lastmode, 
            cpu->r.a, cpu->r.x, cpu->r.y, cpu->r.status, cpu->r.sp, cpu->clockCount);

        /* Exec instruction and get no. of cycles */
        penaltyop = 0;
        penaltyaddr = 0;
        cpu->clockticks = optable[cpu->opID].ticks;

        (*optable[cpu->opID].addrmode)();
        (*optable[cpu->opID].op)();

        if (penaltyop && penaltyaddr) cpu->clockticks++;

        /* Reset the unused flag */
        cpu->r.status |= FLAG_CONSTANT;
        cpu->instructions++;
    }
    cpu->clockticks--;
    cpu->clockCount++;
}

//a few general functions used by various other functions
inline void push16 (uint16_t pushval) 
{
    cpu_write (BASE_STACK + cpu->r.sp, (pushval >> 8) & 0xFF);
    cpu_write (BASE_STACK + ((cpu->r.sp - 1) & 0xFF), pushval & 0xFF);
    cpu->r.sp -= 2;
}

inline void push8 (uint8_t pushval) 
{
    cpu_write (BASE_STACK + cpu->r.sp--, pushval);
}

inline uint16_t pull16() 
{
    uint16_t temp16;
    temp16 = cpu_read (BASE_STACK + ((cpu->r.sp + 1) & 0xFF)) | ((uint16_t) cpu_read(BASE_STACK + ((cpu->r.sp + 2) & 0xFF)) << 8);
    cpu->r.sp += 2;
    return(temp16);
}

inline uint8_t pull8() 
{
    cpu->r.sp++;
    return (cpu_read (BASE_STACK + cpu->r.sp));
}

/* addressing mode functions, calculates effective addresses */

void acc() { //accumulator
    get_addrmode();
}       

void abso() 
{ //absolute
    get_addrmode();
    cpu->abs_addr = (uint16_t)cpu_read(cpu->r.pc) | ((uint16_t)cpu_read(cpu->r.pc+1) << 8);
    cpu->r.pc += 2;
}

void absx() 
{ //absolute X
    get_addrmode();
    uint16_t startpage;
    cpu->abs_addr = ((uint16_t)cpu_read(cpu->r.pc) | ((uint16_t)cpu_read(cpu->r.pc+1) << 8));
    startpage = cpu->abs_addr & 0xFF00;
    cpu->abs_addr += (uint16_t) cpu->r.x;

    if (startpage != (cpu->abs_addr & 0xFF00)) { //one cycle penlty for page-crossing on some cpu->opcodes
        penaltyaddr = 1;
    }

    cpu->r.pc += 2;
}

void absy() 
{ //absolute Y
    get_addrmode();
    uint16_t startpage;
    cpu->abs_addr = ((uint16_t)cpu_read(cpu->r.pc) | ((uint16_t)cpu_read(cpu->r.pc+1) << 8));
    startpage = cpu->abs_addr & 0xFF00;
    cpu->abs_addr += (uint16_t) cpu->r.y;

    if (startpage != (cpu->abs_addr & 0xFF00)) { //one cycle penlty for page-crossing on some cpu->opcodes
        penaltyaddr = 1;
    }

    cpu->r.pc += 2;
}

void imm() 
{ //immediate
    get_addrmode();
    cpu->abs_addr = cpu->r.pc++;
}

void impl() 
{ //implied
    get_addrmode();
    cpu->value = cpu->r.a;
}

void ind() 
{ //indirect
    get_addrmode();
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t) cpu_read(cpu->r.pc) | (uint16_t)((uint16_t) cpu_read(cpu->r.pc+1) << 8);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); /* replicate 6502 page-boundary wraparound bug */
    cpu->abs_addr = (uint16_t) cpu_read(eahelp) | ((uint16_t) cpu_read(eahelp2) << 8);
    cpu->r.pc += 2;
}

void idx() 
{ // (indirect,X)
    get_addrmode();
	uint16_t t = cpu_read(cpu->r.pc);
	cpu->r.pc++;

	uint16_t lo = cpu_read((uint16_t)(t + (uint16_t)cpu->r.x) & 0x00FF);
	uint16_t hi = cpu_read((uint16_t)(t + (uint16_t)cpu->r.x + 1) & 0x00FF);

	cpu->abs_addr = (hi << 8) | lo;
}

void idy() 
{ // (indirect),Y
    get_addrmode();
    uint16_t t = cpu_read(cpu->r.pc);
	cpu->r.pc++;

	uint16_t lo = cpu_read(t & 0x00FF);
	uint16_t hi = cpu_read((t + 1) & 0x00FF);

	cpu->abs_addr = (hi << 8) | lo;
	cpu->abs_addr += cpu->r.y;
	
	if ((cpu->abs_addr & 0xFF00) != (hi << 8)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }
}

void rel() 
{ //relative for branch ops (8-bit immediate cpu->value, sign-extended)
    get_addrmode();
    cpu->rel_addr = (uint16_t)cpu_read(cpu->r.pc++);
    if (cpu->rel_addr & 0x80) cpu->rel_addr |= 0xff00;
}

void zp() 
{ //zero-page
    get_addrmode();
    cpu->abs_addr = (uint16_t)cpu_read((uint16_t)cpu->r.pc++);
}

void zpx() 
{ //zero-page X
    get_addrmode();
    cpu->abs_addr = ((uint16_t)cpu_read((uint16_t)cpu->r.pc++) + (uint16_t)cpu->r.x) & 0xff; //zero-page wraparound
}

void zpy() 
{ //zero-page Y
    get_addrmode();
    cpu->abs_addr = ((uint16_t)cpu_read((uint16_t)cpu->r.pc++) + (uint16_t)cpu->r.y) & 0xff; //zero-page wraparound
}

static uint16_t getvalue() 
{
    if (!(optable[cpu->opID].addrmode == impl))
		cpu->value = cpu_read (cpu->abs_addr);

	return cpu->value;
}

static void putvalue(uint16_t saveval) 
{
    if (optable[cpu->opID].addrmode == acc) cpu->r.a = (uint8_t)(saveval & 0xff);
        else cpu_write (cpu->abs_addr, (saveval & 0xff));
}

/* instruction handler functions */

void adc() /* Add with carry */
{
    get_opname(); 

	cpu->value = getvalue();
	cpu->result = (uint16_t) cpu->r.a + (uint16_t) cpu->value + (uint16_t)(cpu->r.status & FLAG_CARRY);
	
    carrycalc(cpu->result);
    zerocalc(cpu->result);
    overflowcalc(cpu->result, cpu->r.a, cpu->value);
    signcalc(cpu->result);
	
	cpu->r.a = cpu->result & 0xff;
	penaltyop = 1;
}

void and() /* AND (with accumulator) */
{
    get_opname();
    penaltyop = 1;
    cpu->value = getvalue();
    cpu->result = (uint16_t) cpu->r.a & cpu->value;

    zerocalc(cpu->result);
    signcalc(cpu->result);

    saveaccum(cpu->result);
}

void asl() /* Arithmetic shift left */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (uint16_t)cpu->value << 1;

    carrycalc(cpu->result);
    zerocalc(cpu->result);
    signcalc(cpu->result);

    putvalue(cpu->result);
}

void bcc() /* Branch on carry clear */
{
    get_opname();
    if ((cpu->r.status & FLAG_CARRY) == 0) branch();
}

void bcs() /* Branch on carry set */
{
    get_opname();
    if ((cpu->r.status & FLAG_CARRY) == FLAG_CARRY) branch();
}

void beq() /* Branch if equal (zero set) */
{
    get_opname();
    if ((cpu->r.status & FLAG_ZERO) == FLAG_ZERO) branch();
}   

void bit() /* Test bits */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (uint16_t)(cpu->r.a & cpu->value);

    zerocalc(cpu->result);

    /* sign and overflow mask */
    cpu->r.status = (cpu->r.status & 0x3f) | (uint8_t)(cpu->value & 0xc0);
}

void bmi() /* Branch on minus (sign set) */
{
    get_opname();
    if ((cpu->r.status & FLAG_SIGN) == FLAG_SIGN) branch();
}

void bne() /* Branch if not equal (zero clear) */
{
    get_opname();
	if ((cpu->r.status & FLAG_ZERO) == 0) branch();
}

void bpl() /* Branch on plus (sign clear) */
{
    get_opname();
    if ((cpu->r.status & FLAG_SIGN) == 0) branch();
}

void brk() /* Break */
{
    get_opname();
    push16 (++cpu->r.pc); //push next instruction address onto stack
    push8 (cpu->r.status | FLAG_BREAK); //push CPU cpu->r.status to stack
    flag_set(FLAG_INTERRUPT);
    cpu->r.pc = (uint16_t)cpu_read(0xFFFE) | ((uint16_t)cpu_read(0xFFFF) << 8);
}

void bvc() /* Branch on overflow clear */
{
    get_opname();
    if ((cpu->r.status & FLAG_OVERFLOW) == 0) branch();
}

void bvs() /* Branch on overflow set */
{
    get_opname();
    if ((cpu->r.status & FLAG_OVERFLOW) == FLAG_OVERFLOW) branch();
}

void clc() /* Clear carry */
{
    get_opname();
    flag_clear(FLAG_CARRY);
}

void cld() /* Clear decimal */
{
    get_opname();
    flag_clear(FLAG_DECIMAL);
}

void cli() /* Clear interrupt disable */
{
    get_opname();
    flag_clear(FLAG_INTERRUPT);
}

void clv() /* Clear overflow */
{
    get_opname();
    flag_clear(FLAG_OVERFLOW);
}

void cmp() /* Compare (with accumulator) */
{
    get_opname();
    penaltyop = 1;
    cpu->value = getvalue();
    cpu->result = (uint16_t) cpu->r.a - (uint16_t) cpu->value;

    if (cpu->r.a >= (uint8_t)(cpu->value & 0xff)) flag_set(FLAG_CARRY);
        else flag_clear(FLAG_CARRY);
    if (cpu->r.a == (uint8_t)(cpu->value & 0xff)) flag_set(FLAG_ZERO);
        else flag_clear(FLAG_ZERO);
    signcalc(cpu->result);
}

void cpx() /* Compare with X */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (uint16_t) cpu->r.x - cpu->value;

    if (cpu->r.x >= (uint8_t)(cpu->value & 0xff)) flag_set(FLAG_CARRY);
        else flag_clear(FLAG_CARRY);
    if (cpu->r.x == (uint8_t)(cpu->value & 0xff)) flag_set(FLAG_ZERO);
        else flag_clear(FLAG_ZERO);
    signcalc(cpu->result);
}

void cpy() /* Compare with Y */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (uint16_t)cpu->r.y - cpu->value;

    if (cpu->r.y >= (uint8_t)(cpu->value & 0xff)) flag_set(FLAG_CARRY);
        else flag_clear(FLAG_CARRY);
    if (cpu->r.y == (uint8_t)(cpu->value & 0xff)) flag_set(FLAG_ZERO);
        else flag_clear(FLAG_ZERO);
    signcalc(cpu->result);
}

void dec() /* Decrement */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = cpu->value - 1;

    zerocalc(cpu->result);
    signcalc(cpu->result);

    putvalue(cpu->result);
}

void dex() /* Decrement X */
{
    get_opname();
    cpu->r.x--;

    zerocalc(cpu->r.x);
    signcalc(cpu->r.x);
}

void dey() /* Decrement Y */
{
    get_opname();
    cpu->r.y--;

    zerocalc(cpu->r.y);
    signcalc(cpu->r.y);
}

void eor() /* Exclusive OR (with accumulator) */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (uint16_t) cpu->r.a ^ cpu->value;

    zerocalc(cpu->result);
    signcalc(cpu->result);

    saveaccum(cpu->result);
    penaltyop = 1;  
}

void inc() /* Increment */
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = cpu->value + 1;

    zerocalc(cpu->result);
    signcalc(cpu->result);

    putvalue(cpu->result);
}

void inx() /* Increment X */
{
    get_opname();
    cpu->r.x++;

    zerocalc(cpu->r.x);
    signcalc(cpu->r.x);
}

void iny() /* Increment Y */
{
    get_opname();
    cpu->r.y++;

    zerocalc(cpu->r.y);
    signcalc(cpu->r.y);
}

void jmp() /* Jump */
{
    get_opname();
    cpu->r.pc = cpu->abs_addr;
}

void jsr() /* Jump subroutine */
{
    get_opname();
    push16 (--cpu->r.pc);
    cpu->r.pc = cpu->abs_addr;
}

void lda() /* Load accumulator */
{
    get_opname();
    penaltyop = 1;
    cpu->value = getvalue();
    cpu->r.a = (uint8_t)(cpu->value & 0xff);

    zerocalc(cpu->r.a);
    signcalc(cpu->r.a);
}

void ldx() /* Load X */
{
    get_opname();
    penaltyop = 1;
    cpu->value = getvalue();
    cpu->r.x = (uint8_t)(cpu->value & 0xff);

    zerocalc(cpu->r.x);
    signcalc(cpu->r.x);
}

void ldy() /* Load Y */
{
    get_opname();
    penaltyop = 1;
    cpu->value = getvalue();
    cpu->r.y = (uint8_t)(cpu->value & 0x00FF);

    zerocalc(cpu->r.y);
    signcalc(cpu->r.y);
}

void lsr() 
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = cpu->value >> 1;

    if (cpu->value & 1) {
        flag_set(FLAG_CARRY);
    } else { 
        flag_clear(FLAG_CARRY);
    }

    zerocalc(cpu->result);
    signcalc(cpu->result);

    putvalue(cpu->result);
}

void nop() 
{
    get_opname();
    switch (cpu->opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyop = 1;
            break;
    }
}

void ora() 
{
    get_opname();
    penaltyop = 1;
    cpu->value = getvalue();
    cpu->result = (uint16_t) cpu->r.a | cpu->value;

    zerocalc(cpu->result);
    signcalc(cpu->result);

    saveaccum(cpu->result);
}

void pha() 
{
    get_opname();
    push8(cpu->r.a);
}

void php() 
{
    get_opname();
    push8(cpu->r.status | FLAG_BREAK | FLAG_CONSTANT);
    cpu->r.status &= (~FLAG_CONSTANT);
}

void pla() 
{
    get_opname();
    cpu->r.a = pull8();

    zerocalc(cpu->r.a);
    signcalc(cpu->r.a);
}

void plp() 
{
    get_opname();
    cpu->r.status = pull8();
	cpu->r.status |= FLAG_CONSTANT;
    cpu->r.status &= (~FLAG_BREAK);
}

void rol() 
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (cpu->value << 1) | (cpu->r.status & FLAG_CARRY);

    carrycalc(cpu->result);
    zerocalc(cpu->result);
    signcalc(cpu->result);

    putvalue(cpu->result);
}

void ror() 
{
    get_opname();
    cpu->value = getvalue();
    cpu->result = (cpu->value >> 1) | ((cpu->r.status & FLAG_CARRY) << 7);

    if (cpu->value & 1) flag_set(FLAG_CARRY);
        else flag_clear(FLAG_CARRY);
    zerocalc(cpu->result);
    signcalc(cpu->result);

    putvalue(cpu->result);
}

void rti() 
{
    get_opname();
    cpu->r.status = pull8();
    cpu->value = pull16();
    cpu->r.pc = cpu->value;
}

void rts() 
{
    get_opname();
    cpu->value = pull16();
    cpu->r.pc = cpu->value + 1;
}

void sbc() /* Subtract with carry */
{
    get_opname();

    cpu->value = (uint16_t)getvalue() ^ 0xff;
    cpu->result = (uint16_t) cpu->r.a + cpu->value + (uint16_t)(cpu->r.status & FLAG_CARRY);

    carrycalc(cpu->result);
    zerocalc(cpu->result);
    overflowcalc(cpu->result, cpu->r.a, cpu->value);
    signcalc(cpu->result);

    saveaccum(cpu->result);
    penaltyop = 1;
}

void sec() 
{
    get_opname();
    flag_set(FLAG_CARRY);
}

void sed() 
{
    get_opname();
    flag_set(FLAG_DECIMAL);
}

void sei() 
{
    get_opname();
    flag_set(FLAG_INTERRUPT);
}

void sta() 
{
    get_opname();
    putvalue(cpu->r.a);
}

void stx() 
{
    get_opname();
    putvalue(cpu->r.x);
}

void sty() 
{
    get_opname();
    putvalue(cpu->r.y);
}

void tax() 
{
    get_opname();
    cpu->r.x = cpu->r.a;

    zerocalc(cpu->r.x);
    signcalc(cpu->r.x);
}

void tay() 
{
    get_opname();
    cpu->r.y = cpu->r.a;

    zerocalc(cpu->r.y);
    signcalc(cpu->r.y);
}

void tsx() 
{
    get_opname();
    cpu->r.x = cpu->r.sp;

    zerocalc(cpu->r.x);
    signcalc(cpu->r.x);
}

void txa() 
{
    get_opname();
    cpu->r.a = cpu->r.x;

    zerocalc(cpu->r.a);
    signcalc(cpu->r.a);
}

void txs() 
{
    get_opname();
    cpu->r.sp = cpu->r.x;
}

void tya() 
{
    get_opname();
    cpu->r.a = cpu->r.y;

    zerocalc(cpu->r.a);
    signcalc(cpu->r.a);
}

/* Unofficial opcodes */

static void lax() {
    get_opname();
    lda();
    ldx();
}

static void sax() {
    get_opname();
    sta();
    stx();
    putvalue (cpu->r.a & cpu->r.x);
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

static void dcp() {
    get_opname();
    dec();
    cmp();
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

static void isc() {
    get_opname();
    inc();
    sbc();
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

static void slo() {
    get_opname();
    asl();
    ora();
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

static void rla() {
    get_opname();
    rol();
    and();
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

static void sre() {
    get_opname();
    lsr();
    eor();
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

static void rra() {
    get_opname();
    ror();
    adc();
    if (penaltyop && penaltyaddr) cpu->clockticks--;
}

/* Opcodes are grouped by type, then fetched later via ordering in the table */

struct Instruction optable[256] = {
    /* (Mostly) control instructions */
       /* 0x0              0x4             0x8              0xc  */
        { brk, impl,7 }, { nop, zp, 3 }, { php, impl,3 }, { nop, abso,4 }, { bpl, rel, 2 }, { nop, zpx, 4 }, { clc, impl, 2 }, { nop, absx, 4 },
        { jsr, abso,6 }, { bit, zp, 3 }, { plp, impl,4 }, { bit, abso,4 }, { bmi, rel, 2 }, { nop, zpx, 4 }, { sec, impl, 2 }, { nop, absx, 4 },
        { rti, impl,6 }, { nop, zp, 3 }, { pha, impl,3 }, { jmp, abso,3 }, { bvc, rel, 2 }, { nop, zpx, 4 }, { cli, impl, 4 }, { nop, absx, 4 },
        { rts, impl,6 }, { nop, zp, 3 }, { pla, impl,4 }, { jmp, ind, 5 }, { bvs, rel, 2 }, { nop, zpx, 4 }, { sei, impl, 2 }, { nop, absx, 4 },
        { nop, imm, 2 }, { sty, zp, 3 }, { dey, impl,2 }, { sty, abso,4 }, { bcc, rel, 2 }, { sty, zpx, 4 }, { tya, impl, 2 }, { nop, absx, 5 },
        { ldy, imm, 2 }, { ldy, zp, 3 }, { tay, impl,2 }, { ldy, abso,4 }, { bcs, rel, 2 }, { ldy, zpx, 4 }, { clv, impl, 2 }, { ldy, absx, 4 },
        { cpy, imm, 2 }, { cpy, zp, 3 }, { iny, impl,2 }, { cpy, abso,4 }, { bne, rel, 2 }, { nop, zpx, 4 }, { cld, impl, 2 }, { nop, absx, 4 },
        { cpx, imm, 2 }, { cpx, zp, 3 }, { inx, impl,2 }, { cpx, abso,4 }, { beq, rel, 2 }, { nop, zpx, 4 }, { sed, impl, 2 }, { nop, absx, 4 },

    /* ALU operations */
       /* 0x1              0x5             0x9              0xd  */
        { ora, idx, 6 }, { ora, zp, 3 }, { ora, imm, 2 }, { ora, abso,4 }, { ora, idy, 5 }, { ora, zpx, 4 }, { ora, absy, 4 }, { ora, absx, 4 },
        { and, idx, 6 }, { and, zp, 3 }, { and, imm, 2 }, { and, abso,4 }, { and, idy, 5 }, { and, zpx, 4 }, { and, absy, 4 }, { and, absx, 4 },
        { eor, idx, 6 }, { eor, zp, 3 }, { eor, imm, 2 }, { eor, abso,4 }, { eor, idy, 5 }, { eor, zpx, 4 }, { eor, absy, 4 }, { eor, absx, 4 },
        { adc, idx, 6 }, { adc, zp, 3 }, { adc, imm, 2 }, { adc, abso,4 }, { adc, idy, 5 }, { adc, zpx, 4 }, { adc, absy, 4 }, { adc, absx, 4 },
        { sta, idx, 6 }, { sta, zp, 3 }, { nop, imm, 2 }, { sta, abso,4 }, { sta, idy, 6 }, { sta, zpx, 4 }, { sta, absy, 5 }, { sta, absx, 5 },
        { lda, idx, 6 }, { lda, zp, 3 }, { lda, imm, 2 }, { lda, abso,4 }, { lda, idy, 5 }, { lda, zpx, 4 }, { lda, absy, 4 }, { lda, absx, 4 },
        { cmp, idx, 6 }, { cmp, zp, 3 }, { cmp, imm, 2 }, { cmp, abso,4 }, { cmp, idy, 5 }, { cmp, zpx, 4 }, { cmp, absy, 4 }, { cmp, absx, 4 },
        { sbc, idx, 6 }, { sbc, zp, 3 }, { sbc, imm, 2 }, { sbc, abso,4 }, { sbc, idy, 5 }, { sbc, zpx, 4 }, { sbc, absy, 4 }, { sbc, absx, 4 },

    /* read-mod-write operations */
       /* 0x2              0x6             0xa              0xe  */
        { nop, impl,2 }, { asl, zp, 5 }, { asl, acc, 2 }, { asl, abso,6 }, { nop, impl,2 }, { asl, zpx, 6 }, { nop, impl, 2 }, { asl, absx, 7 },
        { nop, impl,2 }, { rol, zp, 5 }, { rol, acc, 2 }, { rol, abso,6 }, { nop, impl,2 }, { rol, zpx, 6 }, { nop, impl, 2 }, { rol, absx, 7 },
        { nop, impl,2 }, { lsr, zp, 5 }, { lsr, acc, 2 }, { lsr, abso,6 }, { nop, impl,2 }, { lsr, zpx, 6 }, { nop, impl, 2 }, { lsr, absx, 7 },
        { nop, impl,2 }, { ror, zp, 5 }, { ror, acc, 2 }, { ror, abso,6 }, { nop, impl,2 }, { ror, zpx, 6 }, { nop, impl, 2 }, { ror, absx, 7 },
        { nop, imm, 2 }, { stx, zp, 3 }, { txa, impl,2 }, { stx, abso,4 }, { nop, impl,2 }, { stx, zpy, 4 }, { txs, impl, 2 }, { nop, absy, 5 },
        { ldx, imm, 2 }, { ldx, zp, 3 }, { tax, impl,2 }, { ldx, abso,4 }, { nop, impl,2 }, { ldx, zpy, 4 }, { tsx, impl, 2 }, { ldx, absy, 4 },
        { nop, imm, 2 }, { dec, zp, 5 }, { dex, impl,2 }, { dec, abso,6 }, { nop, impl,2 }, { dec, zpx, 6 }, { nop, impl, 2 }, { dec, absx, 7 },
        { nop, imm, 2 }, { inc, zp, 5 }, { nop, impl,2 }, { inc, abso,6 }, { nop, impl,2 }, { inc, zpx, 6 }, { nop, impl, 2 }, { inc, absx, 7 },

    /* Unofficial opcodes */
       /* 0x3              0x7             0xb              0xf  */
        { slo, idx, 8 }, { slo, zp, 5 }, { nop, imm, 2 }, { slo, abso,6 }, { slo, idy, 8 }, { slo, zpx, 6 }, { slo, absy, 7 }, { slo, absx, 7 },
        { rla, idx, 8 }, { rla, zp, 5 }, { nop, imm, 2 }, { rla, abso,6 }, { rla, idy, 8 }, { rla, zpx, 6 }, { rla, absy, 7 }, { rla, absx, 7 },
        { sre, idx, 8 }, { sre, zp, 5 }, { nop, imm, 2 }, { sre, abso,6 }, { sre, idy, 8 }, { sre, zpx, 6 }, { sre, absy, 7 }, { sre, absx, 7 },
        { rra, idx, 8 }, { rra, zp, 5 }, { nop, imm, 2 }, { rra, abso,6 }, { rra, idy, 8 }, { rra, zpx, 6 }, { rra, absy, 7 }, { rra, absx, 7 },
        { sax, idx, 6 }, { sax, zp, 3 }, { sax, imm, 2 }, { sax, abso,4 }, { nop, idy, 6 }, { sax, zpy, 4 }, { nop, absy, 5 }, { sax, absy, 5 },
        { lax, idx, 6 }, { lax, zp, 3 }, { lax, imm, 2 }, { lax, abso,4 }, { lax, idy, 5 }, { lax, zpy, 4 }, { nop, absy, 4 }, { lax, absy, 4 },
        { dcp, idx, 8 }, { dcp, zp, 5 }, { nop, imm, 2 }, { dcp, abso,6 }, { dcp, idy, 8 }, { dcp, zpx, 6 }, { dcp, absy, 7 }, { dcp, absx, 7 },
        { isc, idx, 8 }, { isc, zp, 5 }, { sbc, imm, 2 }, { isc, abso,6 }, { isc, idy, 8 }, { isc, zpx, 6 }, { isc, absy, 7 }, { isc, absx, 7 }
};

void nmi6502() 
{
    push16 (cpu->r.pc);
    push8(cpu->r.status);
    cpu->r.status |= FLAG_INTERRUPT;
    cpu->r.pc = (uint16_t)cpu_read(0xfffa) | ((uint16_t)cpu_read(0xfffb) << 8);
}

void irq6502() 
{
    push16 (cpu->r.pc);
    push8(cpu->r.status);
    cpu->r.status |= FLAG_INTERRUPT;
    cpu->r.pc = (uint16_t)cpu_read(0xfffe) | ((uint16_t)cpu_read(0xffff) << 8);
}

void cpu_disassemble (Bus * const bus, uint16_t const start, uint16_t const end)
{
    uint16_t addr = start;
	uint8_t  value = 0x00, lo = 0x00, hi = 0x00;

    cpu->debug = 1;

    while (addr <= end)
	{
        char sInst[64] = "";
        char textbuf[64];

		/* Prefix line with instruction address */
		/* Read instruction, and get its readable name */
		uint8_t opcode = bus_read (bus, addr);
        const uint8_t opID = ((opcode & 3) * 0x40) + (opcode >> 2);
        (*optable[opID].op)();

        sprintf(textbuf, "$%04x: %02x ", addr, opID);
		strcat(sInst, textbuf);

        to_upper(cpu->lastop);
        addr++;

		if ((*optable[opID].addrmode) == &impl)
		{
            sprintf(textbuf, ".. ..  %s       {IMP}", cpu->lastop);
		}
		else if ((*optable[opID].addrmode) == &imm)
		{
			value = bus_read (bus, addr++);
            sprintf(textbuf, "%02x ..  %s #$%02x  {IMM}", value, cpu->lastop, value);
		}
		else if ((*optable[opID].addrmode) == &zp)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;												
			sprintf(textbuf, "%02x ..  %s $%02x {ZP}", lo, cpu->lastop, lo);
		}
		else if ((*optable[opID].addrmode) == &zpx)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;														
			sprintf(textbuf, "%02x ..  %s $%02x, X {ZPX}", lo, cpu->lastop, lo);
		}
		else if ((*optable[opID].addrmode) == &zpy)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;														
			sprintf(textbuf, "%02x ..  %s $%02x, Y {ZPY}", lo, cpu->lastop, lo);
		}
		else if ((*optable[opID].addrmode) == &idx)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;								
			sprintf(textbuf, "%02x ..  %s $%02x, X {IDX}", lo, cpu->lastop, lo);
		}
		else if ((*optable[opID].addrmode) == &idy)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;								
			sprintf(textbuf, "%02x ..  %s $%02x, Y {IDY}", lo, cpu->lastop, lo);
		}
		else if ((*optable[opID].addrmode) == &abso)
		{
			lo = bus_read (bus, addr++);
			hi = bus_read (bus, addr++);    
			sprintf(textbuf, "%02x %02x  %s $%04x {ABS}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
		else if ((*optable[opID].addrmode) == &absx)
		{
			lo = bus_read (bus, addr++);
			hi = bus_read (bus, addr++);
			sprintf(textbuf, "%02x %02x  %s $%04x X {ABX}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
		else if ((*optable[opID].addrmode) == &absy)
		{
			lo = bus_read (bus, addr++);
			hi = bus_read (bus, addr++);
            sprintf(textbuf, "%02x %02x  %s $%04x Y {ABY}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
        
		else if ((*optable[opID].addrmode) == &ind)
		{
			lo = bus_read(bus, addr++);
			hi = bus_read(bus, addr++);
            sprintf(textbuf, "%02x %02x  %s ($%04x)   {IND}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
		else if ((*optable[opID].addrmode) == &rel)
		{
			value = bus_read(bus, addr++);
            sprintf(textbuf, "%02x ..  %s $%04x {REL}", value, cpu->lastop, addr + (int8_t)value);
		}
    
		// Add the formed string to a std::map, using the instruction's
		// address as the key. This makes it convenient to look for later
		// as the instructions are variable in length, so a straight up
		// incremental index is not sufficient.
		//mapLines[line_addr] = sInst;

        /* Add instruction string to the printout */
        strcat(sInst, textbuf);

        sprintf(textbuf, "\n");
		strcat(sInst, textbuf);

        printf("%s", sInst);
	}

    cpu->debug = 0;

	//return mapLines;
}