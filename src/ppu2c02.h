#include <stdio.h>
#include <stdint.h>
#include "gfx/gl_gen.h"
#include "gfx/text.h"

typedef struct PPU2C02_Struct 
{
    /* PPU registers */

    /* PPU memory */
    uint8_t  patternTables[2][4096];

    /* Clock info */
    uint64_t clockCount, clockgoal;
    uint16_t cycle, scanline;

    GLuint fbufferVAO, fbufferVBO;
    GLuint texture;
    Shader fbufferShader;
}
PPU2C02;

void ppu_reset     (PPU2C02 * const ppu);
void ppu_clock     (PPU2C02 * const ppu);
void ppu_exec      (PPU2C02 * const ppu, uint32_t const tickcount);

uint8_t ppu_read      (PPU2C02 * const ppu, uint16_t address);
uint8_t ppu_cpu_read  (PPU2C02 * const ppu, uint16_t const address);
void    ppu_write     (PPU2C02 * const ppu, uint16_t const address);
void    ppu_cpu_write (PPU2C02 * const ppu, uint16_t const address);

void ppu_debug        (PPU2C02 * const ppu, int32_t const scrWidth, int32_t const scrHeight);
void dump_pattern_table();