#include <stdio.h>
#include <stdint.h>
#include "gfx/gl_gen.h"
#include "gfx/text.h"

typedef struct PPU2C02_Struct 
{
    enum ppuRegisters 
    {
        PPU_CONTROL = 0,
        PPU_MASK    = 1,
        PPU_STATUS  = 2,
        OAM_ADDRESS = 3,
        OAM_DATA    = 4,
        PPU_SCROLL  = 5,
        PPU_ADDRESS = 6,
        PPU_DATA    = 7
    }
    ppuRegisters;

    enum ppuControl 
    {
        NAMETABLE1             = 0x01,
        NAMETABLE2             = 0x02,
        VRAM_ADD_INCREMENT     = 0x04,
        SPRITE_PATTERN_ADDR    = 0x08,
        BACKROUND_PATTERN_ADDR = 0x10,
        SPRITE_SIZE            = 0x20,
        MASTER_SLAVE_SELECT    = 0x40,
        GENERATE_VBLANK_NMI    = 0x80
    }
    ppuControl;

    enum ppuMask {
        GRAYSCALE           = 0x01,
        RENDER_BG_LEFT      = 0x02,
        RENDER_SPRITES_LEFT = 0x04,
        RENDER_BG           = 0x08,
        RENDER_SPRITES      = 0x10,
        ENHANCE_RED         = 0x20,
        ENHANCE_GREEN       = 0x40,
        ENHANCE_BLUE        = 0x80

    }
    ppuMask;

    enum ppuStatus {
        SPRITE_OVERFLOW = 0x20,
        SPRITE_ZERO_HIT = 0x40,
        VERTICAL_BLANK  = 0x80
    }
    ppuStatus;

    /* Internal state */
    uint8_t  hiByte, dataBuffer;
    uint8_t  control, mask, status;
    uint8_t  nmi;

    /* PPU memory, with direct access to CHR data */
    uint8_t *romCHR;
    uint8_t  patternTables[2 * 4096];
    uint8_t  nameTables[4 * 1024];
    uint8_t  OAMdata[256];
    uint8_t  paletteTable[32];
    uint16_t VRam, tmpVRam;

    /* Clock info and helpers */
    int16_t  cycle, scanline;
    uint64_t clockCount, clockgoal;
    uint32_t frame;
    uint8_t  mirroring;

    GLuint  fbufferTexture, pTableTexture;
    Shader  fbufferShader;
    uint8_t patternTable[2][128 * 128];
    uint8_t frameBuffer[256 * 240];
}
PPU2C02;

/* Shaders used and 2C03/2C05 color palette */

extern const char *ppu_vs_source, *ppu_fs_source;
extern const uint16_t palette2C03[64];

/* For drawing quads (lazy loading) */

extern uint32_t quadVAO;
extern uint32_t quadVBO;
extern float    quadVertices[];

/* Forward declare ROM */

typedef struct NESrom_struct NESrom;

void ppu_reset     (PPU2C02 * const ppu, NESrom * const rom);
void ppu_clock     (PPU2C02 * const ppu);
void ppu_exec      (PPU2C02 * const ppu, uint32_t const tickcount);

uint8_t ppu_read           (PPU2C02 * const ppu, uint16_t address);
void    ppu_write          (PPU2C02 * const ppu, uint16_t address, uint8_t const data);
uint8_t ppu_register_read  (PPU2C02 * const ppu, uint16_t const address);
void    ppu_register_write (PPU2C02 * const ppu, uint16_t const address, uint8_t const data);

void ppu_render_bg      (PPU2C02 * const ppu);
void ppu_render_sprites (PPU2C02 * const ppu);
void ppu_debug          (PPU2C02 * const ppu, int32_t const scrWidth, int32_t const scrHeight);
void copy_pattern_table (PPU2C02 * const ppu, uint8_t const i);