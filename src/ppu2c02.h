#ifndef PPU_H
#define PPU_H

#include <stdio.h>
#include <stdint.h>
#include "mapper.h"

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

    union
    {
        struct
        {
            uint8_t NAMETABLE_1             : 1;
            uint8_t NAMETABLE_2             : 1;
            uint8_t VRAM_ADD_INCREMENT      : 1;
            uint8_t SPRITE_PATTERN_ADDR     : 1;
            uint8_t BACKGROUND_PATTERN_ADDR : 1;
            uint8_t SPRITE_SIZE             : 1;
            uint8_t MASTER_SLAVE_SELECT     : 1;
            uint8_t ENABLE_NMI              : 1;
        };
        uint8_t flags;
    }
    control;

    union
    {
        struct
        {
            uint8_t GRAYSCALE           : 1;
            uint8_t RENDER_BG_LEFT      : 1;
            uint8_t RENDER_SPRITES_LEFT : 1;
            uint8_t RENDER_BG           : 1;
            uint8_t RENDER_SPRITES      : 1;
            uint8_t ENHANCE_RED         : 1;
            uint8_t ENHANCE_GREEN       : 1;
            uint8_t ENHANCE_BLUE        : 1;
        };
        uint8_t flags;
    }
    mask;

    union
    {
        struct /* DCBAAAAA */
        {
            uint8_t STATUS_UNUSED   : 5;
            uint8_t SPRITE_OVERFLOW : 1;
            uint8_t SPRITE_ZERO_HIT : 1;
            uint8_t VERTICAL_BLANK  : 1;
        };
        uint8_t flags;
    }
    status;

    union
	{
		struct /* xEEEDCBB BBBAAAAA */
		{
			uint16_t coarseX : 5;
			uint16_t coarseY : 5;
			uint16_t nametableX : 1;
			uint16_t nametableY : 1;
			uint16_t fineY : 3;
			uint16_t unused : 1;
		};
		uint16_t reg;
	}
    VRam, tmpVRam;

    /* Internal state */
    uint8_t  latch, dataBuffer;
    uint8_t  fineX;
    uint8_t  nmi;

    /* PPU memory, with direct access to CHR data */
    uint8_t  nameTables[2048];
    uint8_t  VRamData[8192];
    uint8_t  OAMdata[256];
    uint8_t  paletteTable[32];
    uint8_t  OAMaddress;
    Mapper  *mapper;

    /* Temp storage */
    struct NextTile_struct
    {
        uint16_t index;
        uint8_t  lowByte; 
        uint8_t  highByte;    
    }
    nextTile;

	uint16_t bg_shifter_pattern_lo;
	uint16_t bg_shifter_pattern_hi;

    /* Clock info and helpers */
    int16_t  cycle, scanline;
    uint64_t clockCount, clockGoal;
    uint32_t frame;
    uint8_t  mirroring;
    uint8_t  debug;

    uint8_t pTableDebug[2][128 * 128 * 3];
    uint8_t frameBuffer[256 * 256 * 3];
}
PPU2C02;

/* Forward declare ROM */

typedef struct NESrom_struct NESrom;

/* Exec and I/O functions */

void    ppu_reset     (PPU2C02 * const ppu, NESrom * const rom);
void    ppu_clock     (PPU2C02 * const ppu);
void    ppu_exec      (PPU2C02 * const ppu, uint32_t const tickcount);

uint8_t ppu_read           (PPU2C02 * const ppu, uint16_t address);
void    ppu_write          (PPU2C02 * const ppu, uint16_t address, uint8_t const data);
uint8_t ppu_register_read  (PPU2C02 * const ppu, uint16_t const address);
void    ppu_register_write (PPU2C02 * const ppu, uint16_t const address, uint8_t const data);
void    ppu_oam_dma_write  (PPU2C02 * const ppu, uint8_t const data);
 
/* Debug and draw functions */

void ppu_set_pixel         (PPU2C02 * const ppu, uint16_t const x, uint16_t const y);
void copy_pattern_table    (PPU2C02 * const ppu, uint8_t const i);

inline uint8_t ppu_show_debug   (PPU2C02 * const ppu) { return ppu->debug; }
inline void    ppu_toggle_debug (PPU2C02 * const ppu) { ppu->debug = ~ppu->debug; }

#endif