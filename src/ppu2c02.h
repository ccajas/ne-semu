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
        struct 
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
		struct
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
    uint8_t *romCHR;
    uint8_t  patternTables[2][4096];
    uint8_t  nameTables[4][1024];
    uint8_t  OAMdata[256];
    uint8_t  paletteTable[32];
    uint8_t  OAMaddress;

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
    uint64_t clockCount, clockgoal;
    uint32_t frame;
    uint8_t  mirroring;
    uint8_t  debug;

    GLuint  fbufferTexture, pTableTexture, paletteTexture;
    Shader  fbufferShader;
    uint8_t pTableDebug[2][128 * 128 * 3];
    uint8_t frameBuffer[256 * 240 * 3];
    uint8_t fullPixels[192];
    uint8_t palette;
}
PPU2C02;

/* Shaders used and 2C03/2C05 color palette */

extern const char    *ppu_vs_source, *ppu_fs_source;
extern const uint16_t palette2C03[64];
extern const uint32_t PPU_CYCLES_PER_FRAME;

/* For drawing quads (lazy loading) */

extern uint32_t quadVAO;
extern uint32_t quadVBO;
extern float    quadVertices[];

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
 
/* Debug and draw functions */

void ppu_set_pixel         (PPU2C02 * const ppu, uint16_t const x, uint16_t const y);
void ppu_debug             (PPU2C02 * const ppu, int32_t const scrWidth, int32_t const scrHeight);
void copy_pattern_table    (PPU2C02 * const ppu, uint8_t const i);

inline uint8_t ppu_show_debug   (PPU2C02 * const ppu) { return ppu->debug; }
inline void    ppu_toggle_debug (PPU2C02 * const ppu) { ppu->debug = ~ppu->debug; }
void           nametable_debug  (PPU2C02 * const ppu, const int index);