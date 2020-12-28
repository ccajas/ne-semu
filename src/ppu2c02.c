#include <string.h>
#include "ppu2c02.h"
#include "rom.h"
#include "gfx/texture.h"

const uint16_t palette2C03[64] = 
{
    0x333, 0x014, 0x006, 0x326, 0x403, 0x503, 0x400, 0x420, 0x320, 0x120, 0x031, 0x040, 0x022, 0,     0, 0,
    0x555, 0x036, 0x027, 0x407, 0x507, 0x704, 0x620, 0x630, 0x430, 0x140, 0x040, 0x053, 0x044, 0,     0, 0,
    0x777, 0x357, 0x447, 0x637, 0x707, 0x737, 0x743, 0x750, 0x660, 0x360, 0x070, 0x276, 0x077, 0x222, 0, 0,
    0x777, 0x567, 0x657, 0x767, 0x747, 0x755, 0x764, 0x772, 0x773, 0x572, 0x473, 0x276, 0x467, 0x555, 0, 0
};

void ppu_reset (PPU2C02 * const ppu, NESrom * const rom)
{
	ppu->control.flags = 0;
	ppu->mask.flags = 0;
	ppu->status.flags = 0;

	ppu->scanline = 0;
	ppu->cycle = ppu->frame = 0;
	ppu->latch = 0;
	ppu->fineX = 0;
	ppu->dataBuffer = 0;

	ppu->bg_shifter_pattern_lo = ppu->bg_shifter_pattern_hi = 0;
	ppu->VRam.reg = ppu->tmpVRam.reg = 0x0;

	memset(&ppu->nameTables, 0, 2048);
	memset(&ppu->VRamData, 0, 8192);

	/* Copy the first 8K of CHR data as needed for mapper 0 */

	ppu->mirroring = rom->mirroring;
	ppu->mapper    = &rom->mapper;

	if (rom->CHRdata.total && rom->mapperID == 0)
	{
		memcpy(ppu->VRamData, rom->CHRdata.data, rom->CHRdata.total);
        copy_pattern_table (ppu, 0);
        copy_pattern_table (ppu, 1);
	}
}

/* Read PPU register address */

uint8_t ppu_register_read (PPU2C02 * const ppu, uint16_t const address)
{
	assert (address <= 0x7);
	uint8_t data = 0x00;

	switch (address)
	{
		case PPU_STATUS:
			data = (ppu->status.flags & 0xe0) | (ppu->dataBuffer & 0x1f);
			ppu->status.VERTICAL_BLANK = 0;
			ppu->VRam.reg = ppu->tmpVRam.reg;
			ppu->latch = 0;
			break;
		case OAM_DATA:
			data = ppu->OAMdata[ppu->OAMaddress];
			break;
		case PPU_DATA:
			/* Reads here are delayed, retrieve from the buffer */
			data = ppu->dataBuffer;
			ppu->dataBuffer = ppu_read(ppu, ppu->VRam.reg);

			/* Fetch immediately if address is palette data */
			if (ppu->VRam.reg >= 0x3f00) 
				data = ppu->dataBuffer;

			ppu->VRam.reg += (ppu->control.VRAM_ADD_INCREMENT) ? 32 : 1;
			break;
		default: /* PPU control ($2000), mask ($2001), OAM address ($2003), scroll ($2005), PPU address ($2006) not readable */
			break;
	}

	return data;
}

void ppu_register_write (PPU2C02 * const ppu, uint16_t const address, uint8_t const data)
{
	assert (address <= 0x7);

	/* Some write registers are not ready during power up time */
	/*
	const uint16_t POWERUP_CYCLES = 29658;
	switch (address)
	{
		case PPU_CONTROL: case PPU_MASK:
		case PPU_SCROLL:  case PPU_ADDRESS:
			if (ppu->clockCount < POWERUP_CYCLES) return;
	}*/

	switch (address)
	{
		case PPU_CONTROL:
			ppu->control.flags = data;
			ppu->tmpVRam.nametableX = ppu->control.NAMETABLE_1;
			ppu->tmpVRam.nametableY = ppu->control.NAMETABLE_2;
			break;
		case PPU_MASK:
			ppu->mask.flags = data;
			break;
		case OAM_ADDRESS:
			ppu->OAMaddress = data;
			break;
		case OAM_DATA:
			if (ppu->scanline > 239 && ppu->scanline != 241)
				ppu->OAMdata[ppu->OAMaddress] = data;
			break;
		case PPU_SCROLL:
			if (ppu->latch == 0)
			{
				/* Write coarse & fine X values */
				ppu->fineX = data & 0x07;
				ppu->tmpVRam.coarseX = data >> 3;
				ppu->latch = 1;
			}
			else
			{
				/* Write coarse & fine Y values */
				ppu->tmpVRam.fineY = data & 0x7;
				ppu->tmpVRam.coarseY = data >> 3;
				ppu->latch = 0;
			}
			break;
		case PPU_ADDRESS:
			if (ppu->latch == 0)
			{
				/* Write low or high byte alternating */
				ppu->tmpVRam.reg &= 0xff;
				ppu->tmpVRam.reg |= (uint16_t)((data & 0x3f) << 8);
				ppu->latch = 1;
			}
			else
			{
				ppu->tmpVRam.reg = (ppu->tmpVRam.reg & 0xff00) + data;
				ppu->VRam = ppu->tmpVRam;
				ppu->latch = 0;
			}
			break;
		case PPU_DATA:
			ppu_write (ppu, ppu->VRam.reg, data);
			ppu->VRam.reg += (ppu->control.VRAM_ADD_INCREMENT) ? 32 : 1;
			ppu->VRam.reg = ppu->VRam.reg & 0x3fff;
			break;
		default: /* PPU status ($2002) not writable */
			break;
	}
}

uint8_t ppu_read (PPU2C02 * const ppu, uint16_t address)
{
	/* Address should be mapped to lowest 16KB */
	assert (address <= 0x3fff);
	assert (ppu->mapper);

	switch (address)
	{
		case 0 ... 0x1fff:
			/* Read from CHR pattern table */
			return ppu->mapper->read (ppu->mapper, address, 1);
	
		case 0x2000 ... 0x2fff:
			/* Read from nametable data (mirrored every 4KB) */
			if (ppu->mirroring == MIRROR_HORIZONTAL)
			{
				if (address >= 0x2400 && address <= 0x27ff)
					address -= 0x400;
				if (address >= 0x2800 && address <= 0x2bff)
					address -= 0x400;
				if (address >= 0x2c00 && address <= 0x2fff)
					address -= 0x800;
			}
			if (ppu->mirroring == MIRROR_VERTICAL)
			{
				if (address >= 0x2800 && address <= 0x2fff)
					address -= 0x800;
			}
			return ppu->nameTables[address - 0x2000];

		case 0x3000 ... 0x3eff:
			return ppu_read (ppu, address - 0x1000);

		case 0x3f00 ... 0x3fff:
			/* Read from palette table data */
			address &= 0x1f;
			if (address == 0x10) address = 0x0;
			if (address == 0x14) address = 0x4;
			if (address == 0x18) address = 0x8;
			if (address == 0x1c) address = 0xc;

			return ppu->paletteTable[address];

		default:
			break;
	}
	return 1;
}

void ppu_write (PPU2C02 * const ppu, uint16_t address, uint8_t const data)
{
	assert (ppu->mapper);

	if (address >= 0 && address <= 0x1fff)
	{
		/* Write to CHR pattern table */
		ppu->mapper->write (ppu->mapper, address, data, 1);
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{
		/* Write to nametable data (mirrored every 4KB) */
		if (ppu->mirroring == MIRROR_HORIZONTAL)
		{
			if (address >= 0x2400 && address <= 0x27ff)
				address -= 0x400;
			if (address >= 0x2800 && address <= 0x2bff)
				address -= 0x400;
			if (address >= 0x2c00 && address <= 0x2fff)
				address -= 0x800;
		}
		if (ppu->mirroring == MIRROR_VERTICAL)
		{
			if (address >= 0x2800 && address <= 0x2fff)
				address -= 0x800;
		}
		ppu->nameTables[address - 0x2000] = data;
	}
	else if (address >= 0x3f00 && address <= 0x3fff)
	{
		/* Write to palette table data */
		address &= 0x1f;
		if (address == 0x10) address = 0x0;
		if (address == 0x14) address = 0x4;
		if (address == 0x18) address = 0x8;
		if (address == 0x1c) address = 0xc;
		ppu->paletteTable[address] = data;
	}
}

void ppu_oam_dma_write (PPU2C02 * const ppu, uint8_t const data)
{
    ppu->OAMdata[ppu->OAMaddress++] = (uint8_t)data;
}

void ppu_pixel (PPU2C02 * const ppu, uint16_t x, uint16_t y, uint16_t color)
{
	if (x > 256 || y > 240) return;
	uint16_t p = (y << 8) + x;

	ppu->frameBuffer[p * 3]   = (uint8_t)(color >> 8) << 5;
	ppu->frameBuffer[p * 3+1] = (uint8_t)(color >> 4) << 5;
	ppu->frameBuffer[p * 3+2] = (uint8_t)(color << 5);
}

void ppu_background (PPU2C02 * const ppu)
{
	uint16_t pTable = (ppu->control.BACKGROUND_PATTERN_ADDR) ? 1 : 0;

	/* Get offset value in memory based on tile position */
	uint8_t baseTable = ppu->control.NAMETABLE_1 | ppu->control.NAMETABLE_2;

	for (int i = 0; i < 1024; i++)
	{
		uint8_t tile = ppu_read(ppu, i + (0x2000 + baseTable * 0x400));
		uint8_t tileX = i % 32;
		uint8_t tileY = i / 32;

		uint16_t offset = (pTable << 12) + (uint16_t)(tile << 4);

		/* Get attribute table info */
		uint16_t attrTableIndex = ((tileY / 4) * 8) + tileX / 4;
		uint8_t  attrByte = ppu_read(ppu, 0x23c0 + attrTableIndex);
		uint8_t  palette  = attrByte >> ((tileY % 4 / 2) << 2 | (tileX % 4 / 2) << 1) & 3;

		for (int row = 0; row < 8; row++)
		{
			uint8_t tile_lsb = ppu_read(ppu, offset + row + 8);
			uint8_t tile_msb = ppu_read(ppu, offset + row);

			for (int col = 0; col < 8; col++)
			{
				/* Combine bitplanes and color the pixel */
				uint8_t index = (tile_msb & 1) + ((tile_lsb & 1) << 1);
				tile_lsb >>= 1;
				tile_msb >>= 1;

				/* Index 0 is transparent, skip pixel drawing */
				//ppu_pixel (ppu, tileX * 8 + (7 - col), tileY * 8 + row, 0);
				if (index == 0) continue;

				uint16_t palColor = palette2C03[ppu_read(ppu, 0x3f00 + (palette << 2) + index) & 0x3f];
				ppu_pixel (ppu, tileX * 8 + (7 - col), tileY * 8 + row, palColor);
			}
		}
	}
}

void ppu_sprites (PPU2C02 * const ppu)
{
	if (!ppu->mask.RENDER_SPRITES) return;

    uint8_t pTable  = (ppu->control.SPRITE_PATTERN_ADDR) ? 1 : 0;

	for (int i = 0; i < sizeof (ppu->OAMdata); i += 4) 
	{
		uint8_t xPos       = ppu->OAMdata[i + 3];
		uint8_t yPos       = ppu->OAMdata[i];
        uint8_t tile       = ppu->OAMdata[i + 1];
        uint8_t attributes = ppu->OAMdata[i + 2];
        
		uint8_t Vflip = (attributes >> 7 & 1) ? 1 : 0;
        uint8_t Hflip = (attributes >> 6 & 1) ? 1 : 0;

        uint8_t palette = (attributes & 3) | 4;
		uint16_t offset = (pTable << 12) + (uint16_t)(tile << 4);

		for (int row = 0; row < 8; row++)
		{
			uint8_t tile_lsb = ppu_read(ppu, offset + row + 8);
			uint8_t tile_msb = ppu_read(ppu, offset + row);

			for (int col = 0; col < 8; col++)
			{
				/* Combine bitplanes and color the pixel */
				uint8_t index = (tile_msb & 1) + ((tile_lsb & 1) << 1);
				tile_lsb >>= 1;
				tile_msb >>= 1;

				/* Index 0 is transparent, skip pixel drawing */
				if (index == 0) continue;

				uint16_t palColor = palette2C03[ppu_read(ppu, 0x3f00 + (palette << 2) + index) & 0x3f];
				uint8_t col1 = (Hflip) ? col : 7 - col;
				uint8_t row1 = (Vflip) ? 7 - row : row;
				ppu_pixel (ppu, xPos + col1, yPos + row1 + 1, palColor);
			}
		}
	}
}

void ppu_background_pixel (PPU2C02 * const ppu, uint16_t const x, uint16_t const y)
{
	if (x >= 256) return;
	if (y >= 240) return;
	//if (!ppu->mask.RENDER_BG) return;

	uint16_t tileX = x / 8;
	uint16_t tileY = y / 8;
	uint16_t row = x % 8;
	uint16_t col = y % 8;

	uint16_t pTable = (ppu->control.BACKGROUND_PATTERN_ADDR) ? 1 : 0;

	/* Get offset value in memory based on tile position */
	uint8_t baseTable = ppu->control.NAMETABLE_1 | ppu->control.NAMETABLE_2;
	uint8_t tile = ppu_read(ppu, (tileY * 32 + tileX) + (0x2000 + baseTable * 0x400));
	uint8_t xPos = tileX * 8;
	uint8_t yPos = tileY * 8;

	/* Get attribute table info */
	uint16_t attrTableIndex = ((tileY / 4) * 8) + tileX / 4;
	uint8_t  attrByte = ppu_read(ppu, 0x23c0 + attrTableIndex);
	uint8_t  palette  = attrByte >> ((tileY % 4 / 2) << 2 | (tileX % 4 / 2) << 1) & 3;

	/* Combine bitplanes and color the pixel */
	uint16_t offset   = (pTable << 12) + (uint16_t)(tile << 4);
	uint8_t  tile_lsb = ppu_read(ppu, offset + row + 8) >> (7 - col);
	uint8_t  tile_msb = ppu_read(ppu, offset + row)     >> (7 - col);
	uint8_t  index    = (tile_msb & 1) + ((tile_lsb & 1) << 1);

	uint16_t palColor = palette2C03[ppu_read(ppu, 0x3f00 + (palette << 2) + index) & 0x3f];
	//uint16_t p = ((yPos + row - (ppu->tmpVRam.coarseY * 8) - ppu->tmpVRam.fineY) << 8) + 
	//	(xPos + col - (ppu->tmpVRam.coarseX * 8) - ppu->fineX);

	ppu_pixel (ppu, xPos + col, yPos + row, palColor);
}

inline void ppu_nametable_fetch (PPU2C02 * const ppu)
{
	ppu->nextTile.index = ppu_read (ppu, 0x2000 | (ppu->VRam.reg & 0xfff));
}

inline void ppu_attribute_fetch (PPU2C02 * const ppu)
{
	/* To be implemented */	
}

inline void LoadBackgroundShifters (PPU2C02 * const ppu)
{
	/* To be implemented */
}

const uint32_t PPU_CYCLES_PER_FRAME = 89342; /* 341 cycles per 262 scanlines */

void ppu_clock (PPU2C02 * const ppu)
{
	/*
	  Line 
	     0 --- Pre-line, start config
		 1 --- First visible
		 |
	   240 --- Last visible
	   241 --- Post render
	   242 --- Vblank, enable NMI
	   261 --- Last line
	*/

	if (ppu->scanline >= 0 && ppu->scanline < 240)
	{		
		if (ppu->scanline == 1 && ppu->cycle == 0) {
			if (ppu->frame & 1) ppu->cycle = 1;
		}
		
		if (ppu->scanline == 0 && ppu->cycle == 1) {
			ppu->status.VERTICAL_BLANK = 0;
		}

		/* Do more PPU reads (not used) */
		if (ppu->cycle == 337 || ppu->cycle == 339) {
			ppu->nextTile.index = ppu_read(ppu, 0x2000 | (ppu->VRam.reg & 0x0fff));
		}
	}

	ppu_background_pixel (ppu, ppu->cycle, ppu->scanline);

	if (ppu->scanline == 242 && ppu->cycle == 1)
	{
		ppu->status.VERTICAL_BLANK = 1;
		if (ppu->control.ENABLE_NMI) 
			ppu->nmi = 1;

		//ppu_background (ppu);
		ppu_sprites (ppu);
	}

	/* General scan/cycle counting */
	ppu->clockCount++;
	ppu->cycle++;

	if (ppu->cycle % 341 == 0)
	{
		ppu->cycle = 0;
		ppu->scanline++;

		if (ppu->scanline > 261)
		{
			ppu->scanline = 0;
			ppu->nmi = 0;
			ppu->frame++;
		}
	}
}

void copy_pattern_table (PPU2C02 * const ppu, uint8_t const i) 
{
	for (uint16_t tile = 0; tile < 256; tile++)
	{
		/* Get offset value in memory based on tile position */
		uint16_t offset = (i << 12) + ((tile / 16) << 8) + ((tile % 16) << 4);

		/* Loop through each row of a tile */
		for (uint16_t row = 0; row < 8; row++)
		{
			uint8_t tile_lsb = ppu_read (ppu, offset + row);
			uint8_t tile_msb = ppu_read (ppu, offset + row + 8);

			/* Loop through a single row of the bit planes and combine values */
			for (uint16_t col = 0; col < 8; col++)
			{
				/* Get rightmost bit from each byte in memory */
				uint8_t pixel = (tile_lsb & 1) + ((tile_msb & 1) << 1);

				/* X axis needs to be flipped */
				uint16_t pX = ((tile & 0xf) << 3) + (7 - col);
				uint16_t pY = ((tile >> 4) << 3)  + row;

				/* Add to pattern table, with a shade based on pixel value */
				ppu->pTableDebug[i][(pY * 128 + pX) * 3]     =
				ppu->pTableDebug[i][(pY * 128 + pX) * 3 + 1] =
				ppu->pTableDebug[i][(pY * 128 + pX) * 3 + 2] = pixel * 0x55;

				tile_lsb >>= 1;
				tile_msb >>= 1;
			}
		}
	}
}