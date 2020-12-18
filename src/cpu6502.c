/* Fake6502 CPU emulator core v1.1 *******************
 * (c)2011-2013 Mike Chambers                        *
 *****************************************************/

#include <string.h>
#include "bus.h"

//externally supplied functions
uint8_t cpu_read (uint16_t address)                { return bus_read(&bus, address);  }
void    cpu_write(uint16_t address, uint8_t value) { bus_write(&bus, address, value); }

/*
*=$8000
	LDX #10 
	STX $0000
    LDX #3
    STX $0001
    LDY $0000
    LDA #0
    CLC
    loop
    ADC $0001
    DEY
    BNE loop
    STA $0002
    NOP
    NOP
    NOP
*/

const uint8_t testProg[28] = {
    0xA2, 0x0A, 0x8E, 0x0,  0x0,  0xA2, 0x03, 0x8E, 
    0x01, 0x0,  0xAC, 0x0,  0x0,  0xA9, 0x0,  0x18, 
    0x6D, 0x01, 0x00, 0x88, 0xD0, 0xFA, 0x8D, 0x02,
    0x00, 0xEA, 0xEA, 0xEA
};

const uint16_t
    CPU_RAM_START   = 0,
    PPU_REG_START   = 0x2000,
    APU_IO_START    = 0x4000,
    APU_IO_DISABLED = 0x4018,
    PRG_START       = 0x4020,
    PRG_RAM_START   = 0x6000,
    PRG_ROM_START   = 0x8000,
    PRG_ROM_END     = 0xffff;

//6502 defines
CPU6502 *cpu = &bus.cpu;

#define saveaccum(n) cpu->r.a = (uint8_t)((n) & 0xff)

/* flag modifier macros */
#define flag_set(f)   cpu->r.status |= f
#define flag_clear(f) cpu->r.status &= (~f)

/* Meta function macros (for disassembly/debugging) */

#define get_opname()   if (cpu->getlastop) { sprintf(cpu->lastop,   "%s",  __func__); return; } 
#define get_addrmode() if (cpu->getlastop) { sprintf(cpu->lastmode, "%4s", __func__); return; } 
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

static void (*addrtable[256])();
//static struct Instruction optable[][64];

uint8_t penaltyop, penaltyaddr;

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
    if (!(addrtable[cpu->opcode] == impl))
		cpu->value = cpu_read (cpu->abs_addr);

	return cpu->value;
}

static void putvalue(uint16_t saveval) 
{
    if (addrtable[cpu->opcode] == acc) cpu->r.a = (uint8_t)(saveval & 0xff);
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

#ifdef usetables

static void (*addrtable[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */    impl, idx,  impl, idx,   zp,   zp,   zp,   zp,  impl,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
/* 1 */     rel, idy,  impl, idy,  zpx,  zpx,  zpx,  zpx,  impl, absy, impl, absy, absx, absx, absx, absx, /* 1 */
/* 2 */    abso, idx,  impl, idx,   zp,   zp,   zp,   zp,  impl,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
/* 3 */     rel, idy,  impl, idy,  zpx,  zpx,  zpx,  zpx,  impl, absy, impl, absy, absx, absx, absx, absx, /* 3 */
/* 4 */    impl, idx,  impl, idx,   zp,   zp,   zp,   zp,  impl,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
/* 5 */     rel, idy,  impl, idy,  zpx,  zpx,  zpx,  zpx,  impl, absy, impl, absy, absx, absx, absx, absx, /* 5 */
/* 6 */    impl, idx,  impl, idx,   zp,   zp,   zp,   zp,  impl,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
/* 7 */     rel, idy,  impl, idy,  zpx,  zpx,  zpx,  zpx,  impl, absy, impl, absy, absx, absx, absx, absx, /* 7 */
/* 8 */     imm, idx,  imm,  idx,   zp,   zp,   zp,   zp,  impl,  imm, impl,  imm, abso, abso, abso, abso, /* 8 */
/* 9 */     rel, idy,  impl, idy,  zpx,  zpx,  zpy,  zpy,  impl, absy, impl, absy, absx, absx, absy, absy, /* 9 */
/* A */     imm, idx,  imm,  idx,   zp,   zp,   zp,   zp,  impl,  imm, impl,  imm, abso, abso, abso, abso, /* A */
/* B */     rel, idy,  impl, idy,  zpx,  zpx,  zpy,  zpy,  impl, absy, impl, absy, absx, absx, absy, absy, /* B */
/* C */     imm, idx,  imm,  idx,   zp,   zp,   zp,   zp,  impl,  imm, impl,  imm, abso, abso, abso, abso, /* C */
/* D */     rel, idy,  impl, idy,  zpx,  zpx,  zpx,  zpx,  impl, absy, impl, absy, absx, absx, absx, absx, /* D */
/* E */     imm, idx,  imm,  idx,   zp,   zp,   zp,   zp,  impl,  imm, impl,  imm, abso, abso, abso, abso, /* E */
/* F */     rel, idy,  impl, idy,  zpx,  zpx,  zpx,  zpx,  impl, absy, impl, absy, absx, absx, absx, absx  /* F */
};   

static void (*optable2[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |      */
/* 0 */      brk,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
/* 1 */      bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
/* 2 */      jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
/* 3 */      bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
/* 4 */      rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
/* 5 */      bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
/* 6 */      rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
/* 7 */      bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
/* 8 */      nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
/* 9 */      bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
/* A */      ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
/* B */      bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
/* C */      cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
/* D */      bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
/* E */      cpx,  sbc,  nop,  isc,  cpx,  sbc,  inc,  isc,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isc, /* E */
/* F */      beq,  sbc,  nop,  isc,  nop,  sbc,  inc,  isc,  sed,  sbc,  nop,  isc,  nop,  sbc,  inc,  isc  /* F */
};

static const uint8_t ticktable[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
/* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
/* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
/* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
/* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
/* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
/* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
/* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
/* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
/* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
/* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
/* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
/* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
/* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
/* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
};
#endif

struct Instruction
{		
    void    (*op)();
    void    (*addrmode)();
    uint8_t ticks;
};

//static void (*optable[256])();

static struct Instruction optable[][64] = {
    /* (Mostly) control instructions */
    {  /* 0x0              0x4             0x8              0xc  */
        { brk, impl,7 }, { nop, zp, 3 }, { php, impl,3 }, { nop, abso,4 }, { bpl, rel, 2 }, { nop, zpx, 4 }, { clc, impl, 2 }, { nop, absx, 4 },
        { jsr, abso,6 }, { bit, zp, 3 }, { plp, impl,4 }, { bit, abso,4 }, { bmi, rel, 2 }, { nop, zpx, 4 }, { sec, impl, 2 }, { nop, absx, 4 },
        { rti, impl,6 }, { nop, zp, 3 }, { pha, impl,3 }, { jmp, abso,3 }, { bvc, rel, 2 }, { nop, zpx, 4 }, { cli, impl, 4 }, { nop, absx, 4 },
        { rts, impl,6 }, { nop, zp, 3 }, { pla, impl,4 }, { jmp, ind, 5 }, { bvs, rel, 2 }, { nop, zpx, 4 }, { sei, impl, 2 }, { nop, absx, 4 },
        { nop, imm, 2 }, { sty, zp, 3 }, { dey, impl,2 }, { sty, abso,4 }, { bcc, rel, 2 }, { sty, zpx, 4 }, { tya, impl, 2 }, { nop, absx, 5 },
        { ldy, imm, 2 }, { ldy, zp, 3 }, { tay, impl,2 }, { ldy, abso,4 }, { bcs, rel, 2 }, { ldy, zpx, 4 }, { clv, impl, 2 }, { ldy, absx, 4 },
        { cpy, imm, 2 }, { cpy, zp, 3 }, { iny, impl,2 }, { cpy, abso,4 }, { bne, rel, 2 }, { nop, zpx, 4 }, { cld, impl, 2 }, { nop, absx, 4 },
        { cpx, imm, 2 }, { cpx, zp, 3 }, { inx, impl,2 }, { cpx, abso,4 }, { beq, rel, 2 }, { nop, zpx, 4 }, { sed, impl, 2 }, { nop, absx, 4 }
    },
    /* ALU operations */
    {  /* 0x1              0x5             0x9              0xd  */
        { ora, idx, 6 }, { ora, zp, 3 }, { ora, imm, 2 }, { ora, abso,4 }, { ora, idy, 5 }, { ora, zpx, 4 }, { ora, absy, 4 }, { ora, absx, 4 },
        { and, idx, 6 }, { and, zp, 3 }, { and, imm, 2 }, { and, abso,4 }, { and, idy, 5 }, { and, zpx, 4 }, { and, absy, 4 }, { and, absx, 4 },
        { eor, idx, 6 }, { eor, zp, 3 }, { eor, imm, 2 }, { eor, abso,4 }, { eor, idy, 5 }, { eor, zpx, 4 }, { eor, absy, 4 }, { eor, absx, 4 },
        { adc, idx, 6 }, { adc, zp, 3 }, { adc, imm, 2 }, { adc, abso,4 }, { adc, idy, 5 }, { adc, zpx, 4 }, { adc, absy, 4 }, { adc, absx, 4 },
        { sta, idx, 6 }, { sta, zp, 3 }, { nop, imm, 2 }, { sta, abso,4 }, { sta, idy, 6 }, { sta, zpx, 4 }, { sta, absy, 5 }, { sta, absx, 5 },
        { lda, idx, 6 }, { lda, zp, 3 }, { lda, imm, 2 }, { lda, abso,4 }, { lda, idy, 5 }, { lda, zpx, 4 }, { lda, absy, 4 }, { lda, absx, 4 },
        { cmp, idx, 6 }, { cmp, zp, 3 }, { cmp, imm, 2 }, { cmp, abso,4 }, { cmp, idy, 5 }, { cmp, zpx, 4 }, { cmp, absy, 4 }, { cmp, absx, 4 },
        { sbc, idx, 6 }, { sbc, zp, 3 }, { sbc, imm, 2 }, { sbc, abso,4 }, { sbc, idy, 5 }, { sbc, zpx, 4 }, { sbc, absy, 4 }, { sbc, absx, 4 }
    },
    /* read-mod-write operations */
    {  /* 0x2              0x6             0xa              0xe  */
        { nop, impl,2 }, { asl, zp, 5 }, { asl, acc, 2 }, { asl, abso,6 }, { nop, impl,2 }, { asl, zpx, 6 }, { nop, impl, 2 }, { asl, absx, 7 },
        { nop, impl,2 }, { rol, zp, 5 }, { rol, acc, 2 }, { rol, abso,6 }, { nop, impl,2 }, { rol, zpx, 6 }, { nop, impl, 2 }, { rol, absx, 7 },
        { nop, impl,2 }, { lsr, zp, 5 }, { lsr, acc, 2 }, { lsr, abso,6 }, { nop, impl,2 }, { lsr, zpx, 6 }, { nop, impl, 2 }, { lsr, absx, 7 },
        { nop, impl,2 }, { ror, zp, 5 }, { ror, acc, 2 }, { ror, abso,6 }, { nop, impl,2 }, { ror, zpx, 6 }, { nop, impl, 2 }, { ror, absx, 7 },
        { nop, imm, 2 }, { stx, zp, 3 }, { txa, impl,2 }, { stx, abso,4 }, { nop, impl,2 }, { stx, zpy, 4 }, { txs, impl, 2 }, { nop, absy, 5 },
        { ldx, imm, 2 }, { ldx, zp, 3 }, { tax, impl,2 }, { ldx, abso,4 }, { nop, impl,2 }, { ldx, zpy, 4 }, { tsx, impl, 2 }, { ldx, absy, 4 },
        { nop, imm, 2 }, { dec, zp, 5 }, { dex, impl,2 }, { dec, abso,6 }, { nop, impl,2 }, { dec, zpx, 6 }, { nop, impl, 2 }, { dec, absx, 7 },
        { nop, imm, 2 }, { inc, zp, 5 }, { nop, impl,2 }, { inc, abso,6 }, { nop, impl,2 }, { inc, zpx, 6 }, { nop, impl, 2 }, { inc, absx, 7 }
    },
    /* Unofficial opcodes */
    {  /* 0x3              0x7             0xb              0xf  */
        { slo, idx, 8 }, { slo, zp, 5 }, { nop, imm, 2 }, { slo, abso,6 }, { slo, idy, 8 }, { slo, zpx, 6 }, { slo, absy, 7 }, { slo, absx, 7 },
        { rla, idx, 8 }, { rla, zp, 5 }, { nop, imm, 2 }, { rla, abso,6 }, { rla, idy, 8 }, { rla, zpx, 6 }, { rla, absy, 7 }, { rla, absx, 7 },
        { sre, idx, 8 }, { sre, zp, 5 }, { nop, imm, 2 }, { sre, abso,6 }, { sre, idy, 8 }, { sre, zpx, 6 }, { sre, absy, 7 }, { sre, absx, 7 },
        { rra, idx, 8 }, { rra, zp, 5 }, { nop, imm, 2 }, { rra, abso,6 }, { rra, idy, 8 }, { rra, zpx, 6 }, { rra, absy, 7 }, { rra, absx, 7 },
        { sax, idx, 6 }, { sax, zp, 3 }, { sax, imm, 2 }, { sax, abso,4 }, { nop, idy, 6 }, { sax, zpy, 4 }, { nop, absy, 5 }, { sax, absy, 5 },
        { lax, idx, 6 }, { lax, zp, 3 }, { lax, imm, 2 }, { lax, abso,4 }, { lax, idy, 5 }, { lax, zpy, 4 }, { nop, absy, 4 }, { lax, absy, 4 },
        { dcp, idx, 8 }, { dcp, zp, 5 }, { nop, imm, 2 }, { dcp, abso,6 }, { dcp, idy, 8 }, { dcp, zpx, 6 }, { dcp, absy, 7 }, { dcp, absx, 7 },
        { isc, idx, 8 }, { isc, zp, 5 }, { sbc, imm, 2 }, { isc, abso,6 }, { isc, idy, 8 }, { isc, zpx, 6 }, { isc, absy, 7 }, { isc, absx, 7 }
    }
};

#ifdef populate
void set_op_(void (*op)(), uint8_t op_arr[]) 
{
    for (int i = 0; op_arr[i] != 0; i++) {
        optable[op_arr[i]] = op;
    }
};

void populate_opcodes()
{
    #define set_op(ins, ...) set_op_(ins, (uint8_t[]){__VA_ARGS__, 0});

    for (int i = 0; i < 255; i++)
        optable[i] = nop;

    /* (Mostly) control instructions */

    optable[0] = brk;
    set_op (php, 0x08);
    set_op (bpl, 0x10);
    set_op (clc, 0x18);
    set_op (jsr, 0x20);
    set_op (bit, 0x24, 0x2c);
    set_op (plp, 0x28);
    set_op (bmi, 0x30);
    set_op (sec, 0x38);
    set_op (rti, 0x40);
    set_op (pha, 0x48);
    set_op (jmp, 0x4c, 0x6c);
    set_op (bvc, 0x50);
    set_op (cli, 0x58);
    set_op (rts, 0x60);
    set_op (pla, 0x68);
    set_op (bvs, 0x70);
    set_op (sei, 0x78);
    set_op (sty, 0x84);
    set_op (sty, 0x8c, 0x94);
    set_op (dey, 0x88);
    set_op (bcc, 0x90);
    set_op (tya, 0x98);
    set_op (ldy, 0xa0, 0xa4, 0xac, 0xb4, 0xbc);
    set_op (tay, 0xa8);
    set_op (bcs, 0xb0);
    set_op (clv, 0xb8);
    set_op (cpy, 0xc0, 0xc4, 0xcc);
    set_op (iny, 0xc8);
    set_op (bne, 0xd0);
    set_op (cld, 0xd8);
    set_op (cpx, 0xe0, 0xe4, 0xec);
    set_op (inx, 0xe8);
    set_op (beq, 0xf0);
    set_op (sed, 0xf8);

    /* ALU operations */

    set_op (ora, 0x01, 0x05, 0x09, 0x0d, 0x11, 0x15, 0x19, 0x1d);
    set_op (and, 0x21, 0x25, 0x29, 0x2d, 0x31, 0x35, 0x39, 0x3d);
    set_op (eor, 0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d);
    set_op (adc, 0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d);
    set_op (sta, 0x81, 0x85,       0x8d, 0x91, 0x95, 0x99, 0x9d);
    set_op (lda, 0xa1, 0xa5, 0xa9, 0xad, 0xb1, 0xb5, 0xb9, 0xbd);
    set_op (cmp, 0xc1, 0xc5, 0xc9, 0xcd, 0xd1, 0xd5, 0xd9, 0xdd);
    set_op (sbc, 0xe1, 0xe5, 0xe9, 0xed, 0xf1, 0xf5, 0xf9, 0xfd);

    /* read-mod-write operations */

    set_op (asl, 0x06, 0x0a, 0x0e, 0x16, 0x1e);
    set_op (rol, 0x26, 0x2a, 0x2e, 0x36, 0x3e);
    set_op (lsr, 0x46, 0x4a, 0x4e, 0x56, 0x5e);
    set_op (ror, 0x66, 0x6a, 0x6e, 0x76, 0x7e);
    set_op (stx, 0x86, 0x8e, 0x96);
    set_op (txa, 0x8a);
    set_op (txs, 0x9a);
    set_op (ldx, 0xa2, 0xa6, 0xae, 0xb6, 0xbe) 
    set_op (tax, 0xaa);  
    set_op (tsx, 0xba);
    set_op (dec, 0xc6, 0xce, 0xd6, 0xde); 
    set_op (dex, 0xca); 
    set_op (inc, 0xe6, 0xee, 0xf6, 0xfe);

    /* Unofficial opcodes */

    set_op (slo, 0x03, 0x07, 0x0f, 0x13, 0x17, 0x1b, 0x1f);
    set_op (rla, 0x23, 0x27, 0x2f, 0x33, 0x37, 0x3b, 0x3f);
    set_op (sre, 0x43, 0x47, 0x4f, 0x53, 0x57, 0x5b, 0x5f);
    set_op (rra, 0x63, 0x67, 0x6f, 0x73, 0x77, 0x7b, 0x7f);
    set_op (sax, 0x83, 0x87, 0x8f, 0x97);
    set_op (lax, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbf);
    set_op (dcp, 0xc3, 0xc7, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf);
    set_op (isc, 0xe3, 0xe7, 0xef, 0xf3, 0xf7, 0xfb, 0xff);
    set_op (sbc, 0xeb);

    #undef set_op
}
#endif

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
    cpu->getlastop = 0;

	/* Takes 7 cycles to reset */
    cpu->clockticks = 7;
}

void cpu_run_instruction (CPU6502 * const cpu)
{
    cpu->lastpc = cpu->r.pc;
    cpu->opcode = cpu_read(cpu->r.pc++);
    cpu->r.status |= FLAG_CONSTANT;

    /* Fetch OP name */
    cpu->getlastop = 1;
    (*optable[cpu->opcode & 3][cpu->opcode >> 2].addrmode)();
    (*optable[cpu->opcode & 3][cpu->opcode >> 2].op)();
    to_upper(cpu->lastop);
    cpu->getlastop = 0;

    printf("$%04x %02x %s (%s) : A:%02x X:%02x Y:%02x P:%02x SP:%02x CYC:%ld\n", 
       cpu->lastpc, cpu->opcode, cpu->lastop, cpu->lastmode, cpu->r.a, cpu->r.x, cpu->r.y, cpu->r.status, cpu->r.sp, cpu->clockCount);

    /* Exec instruction and get no. of cycles */
    penaltyop = 0;
    penaltyaddr = 0;
    cpu->clockticks = optable[cpu->opcode & 3][cpu->opcode >> 2].ticks;

    (*optable[cpu->opcode & 3][cpu->opcode >> 2].addrmode)();
    (*optable[cpu->opcode & 3][cpu->opcode >> 2].op)();

    if (penaltyop && penaltyaddr) cpu->clockticks++;

    /* Reset the unused flag */
    cpu->r.status |= FLAG_CONSTANT;
    cpu->instructions++;
}

void cpu_exec (CPU6502 * const cpu, uint32_t const tickcount)
{
    cpu->clockGoal += tickcount;

    while (cpu->clockCount < cpu->clockGoal) 
    {
        cpu_clock (cpu);
    }
}

void cpu_clock (CPU6502 * const cpu)
{
    if (cpu->clockticks == 0)
    {
        cpu_run_instruction (cpu);
    }
    cpu->clockticks--;
    cpu->clockCount++;
}

void cpu_disassemble (Bus * const bus, uint16_t const start, uint16_t const end)
{
    uint16_t addr = start;
	uint8_t  value = 0x00, lo = 0x00, hi = 0x00;

    cpu->getlastop = 1;

    while (addr <= end)
	{
        char sInst[64] = "";
        char textbuf[64];

		/* Prefix line with instruction address */
		/* Read instruction, and get its readable name */
		uint8_t opcode = bus_read (bus, addr++);
        (*optable[cpu->opcode & 3][cpu->opcode >> 2].op)();

        sprintf(textbuf, "$%04x: %02x ", addr, opcode);
		strcat(sInst, textbuf);

        to_upper(cpu->lastop);
        //sprintf(textbuf, "%02x %s ", opcode, cpu->lastop);
		//strcat(sInst, textbuf);

		// Get oprands from desired locations, and form the
		// instruction based upon its addressing mode. These
		// routines mimmick the actual fetch routine of the
		// 6502 in order to get accurate data as part of the
		// instruction
		if ((*addrtable[opcode]) == &impl)
		{
            sprintf(textbuf, ".. ..  %s       {IMP}", cpu->lastop);
		}
		else if ((*addrtable[opcode]) == &imm)
		{
			value = bus_read (bus, addr++);
            sprintf(textbuf, "%02x ..  %s #$%02x  {IMM}", value, cpu->lastop, value);
		}
		else if ((*addrtable[opcode]) == &zp)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;												
			sprintf(textbuf, "%02x ..  %s $%02x {ZP}", lo, cpu->lastop, lo);
		}
		else if ((*addrtable[opcode]) == &zpx)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;														
			sprintf(textbuf, "%02x ..  %s $%02x, X {ZPX}", lo, cpu->lastop, lo);
		}
		else if ((*addrtable[opcode]) == &zpy)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;														
			sprintf(textbuf, "%02x ..  %s $%02x, Y {ZPY}", lo, cpu->lastop, lo);
		}
		else if ((*addrtable[opcode]) == &idx)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;								
			sprintf(textbuf, "%02x ..  %s $%02x, X {IDX}", lo, cpu->lastop, lo);
		}
		else if ((*addrtable[opcode]) == &idy)
		{
			lo = bus_read(bus, addr++);
			hi = 0x00;								
			sprintf(textbuf, "%02x ..  %s $%02x, Y {IDY}", lo, cpu->lastop, lo);
		}
		else if ((*addrtable[opcode]) == &abso)
		{
			lo = bus_read (bus, addr++);
			hi = bus_read (bus, addr++);    
			sprintf(textbuf, "%02x %02x  %s $%04x {ABS}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
		else if ((*addrtable[opcode]) == &absx)
		{
			lo = bus_read (bus, addr++);
			hi = bus_read (bus, addr++);
			sprintf(textbuf, "%02x %02x  %s $%04x X {ABX}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
		else if ((*addrtable[opcode]) == &absy)
		{
			lo = bus_read (bus, addr++);
			hi = bus_read (bus, addr++);
            sprintf(textbuf, "%02x %02x  %s $%04x Y {ABY}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
        
		else if ((*addrtable[opcode]) == &ind)
		{
			lo = bus_read(bus, addr++);
			hi = bus_read(bus, addr++);
            sprintf(textbuf, "%02x %02x  %s ($%04x)   {IND}", lo, hi, cpu->lastop, (uint16_t)(hi << 8) | lo);
		}
		else if ((*addrtable[opcode]) == &rel)
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

    cpu->getlastop = 0;

	//return mapLines;
}