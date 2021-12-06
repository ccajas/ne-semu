#include <string.h>
#include "ppu2c02.h"
#include "palette.h"
#include "rom.h"

void ppu_reset (PPU2C02 * const ppu, NESrom * const rom)
{
	ppu->control.flags = 0;
	ppu->mask.flags = 0;
	ppu->status.flags = 0;

	ppu->scanline = ppu->cycle = ppu->frame = 0;
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
		case PPU_CONTROL: /* $2000 */
			ppu->control.flags = data;
			ppu->tmpVRam.nametableX = ppu->control.NAMETABLE_1;
			ppu->tmpVRam.nametableY = ppu->control.NAMETABLE_2;
			break;
		case PPU_MASK:    /* $2001 */
			ppu->mask.flags = data;
			break;
		case OAM_ADDRESS: /* $2003 */
			ppu->OAMaddress = data;
			break;
		case OAM_DATA:    /* $2004 */
			if (ppu->scanline > 239 && ppu->scanline != 241)
				ppu->OAMdata[ppu->OAMaddress] = data;
			ppu->VRam.reg++;
			break;
		case PPU_SCROLL:  /* $2005 */
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
		case PPU_ADDRESS: /* $2006 */
			if (ppu->latch == 0) /* First write, high */
			{
				ppu->tmpVRam.reg &= 0xff;
				ppu->tmpVRam.reg |= (uint16_t)((data & 0x3f) << 8);
				ppu->tmpVRam.reg &= 0x3fff;
				ppu->latch = 1;
			}
			else /* Second write, low */
			{
				ppu->tmpVRam.reg &= 0xff00;
				ppu->tmpVRam.reg |= data;
				ppu->VRam = ppu->tmpVRam;
				ppu->latch = 0;
			}
			break;
		case PPU_DATA:   /* $2007 */
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

			address &= 0xfff;
			/* Read from nametable data (mirrored every 4KB) */
			if (ppu->mirroring == MIRROR_HORIZONTAL)
			{
				if (address >= 0xc00)
					address -= 0x800;
				if (address >= 0x800 || address >= 0x400)
					address -= 0x400;
			}
			if (ppu->mirroring == MIRROR_VERTICAL)
			{
				address &= 0x7ff;
			}
			return ppu->nameTables[address];

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
		address &= 0xfff;
		/* Write to nametable data (mirrored every 4KB) */
		if (ppu->mirroring == MIRROR_HORIZONTAL)
		{
			if (address >= 0xc00)
				address -= 0x800;
			if (address >= 0x800 || address >= 0x400)
				address -= 0x400;
		}
		if (ppu->mirroring == MIRROR_VERTICAL)
		{
			address &= 0x7ff;
		}
		ppu->nameTables[address] = data;
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

#define PPU_PIXEL

#ifdef PPU_PIXEL
void ppu_background (PPU2C02 * const ppu, uint16_t const x, uint16_t const y)
{
	if (!ppu->mask.RENDER_BG) return;

	uint16_t tileX = x / 8;
	uint16_t tileY = y / 8;
	uint16_t row = x % 8;
	uint16_t col = y % 8;

	uint16_t pTable = (ppu->control.BACKGROUND_PATTERN_ADDR) ? 1 : 0;

	/* Get offset value in memory based on tile position */
	uint8_t baseTable = ppu->control.NAMETABLE_1 | ppu->control.NAMETABLE_2;
	uint8_t tile = ppu_read(ppu, (tileY * 32 + tileX) + (0x2000 + baseTable * 0x400));

	/* Get attribute table info */
	uint16_t attrTableIndex = ((tileY / 4) * 8) + tileX / 4;
	uint8_t  attrByte = ppu_read(ppu, 0x23c0 + attrTableIndex);
	uint8_t  palette  = attrByte >> (((tileY & 3) / 2) << 2 | ((tileX & 3) / 2) << 1) & 3;

	/* Combine bitplanes and color the pixel */
	uint16_t offset   = (pTable << 12) + (uint16_t)(tile << 4);
	uint8_t  tile_lsb = ppu_read(ppu, offset + row + 8) >> (7 - col);
	uint8_t  tile_msb = ppu_read(ppu, offset + row)     >> (7 - col);
	uint8_t  index    = (tile_msb & 1) + ((tile_lsb & 1) << 1);

	uint16_t palColor = palette2C03[ppu_read(ppu, 0x3f00 + (palette << 2) + index) & 0x3f];
	int16_t pX = (tileX * 8 + col - (ppu->tmpVRam.coarseX * 8) - ppu->fineX);
	int16_t pY = ((tileY * 8 + row - (ppu->tmpVRam.coarseY * 8) - ppu->tmpVRam.fineY));

	if (pY < 0)   pY += 240;
	if (pY > 239) pY -= 240;
	ppu_pixel (ppu, pX, pY, palColor);
	//ppu_pixel (ppu, xPos + col, yPos + row, palColor);
}
#else
void ppu_background (PPU2C02 * const ppu, uint16_t const x, uint16_t const y)
{
	uint16_t pTable = (ppu->control.BACKGROUND_PATTERN_ADDR) ? 1 : 0;

	/* Get offset value in memory based on tile position */
	uint8_t baseTable = ppu->control.NAMETABLE_2 | ppu->control.NAMETABLE_1;

	for (int i = 0; i < 0x3c0; i++)
	{
		uint16_t yPos = (baseTable > 0) ? 240 : 0;

		/* Read nametable data */
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
				ppu_pixel (ppu, tileX * 8 + (7 - col), yPos, 0);
				if (index == 0) continue;

				uint16_t palColor = palette2C03[ppu_read(ppu, 0x3f00 + (palette << 2) + index) & 0x3f];
				ppu_pixel (
					ppu, tileX * 8 + (7 - col), yPos + (tileY - ppu->VRam.coarseY) * 8 + row - ppu->VRam.fineY, 
					palColor);
			}
		}
	}
}
#endif

void ppu_sprites (PPU2C02 * const ppu, uint16_t const x, uint16_t const y)
{
	if (!ppu->mask.RENDER_SPRITES) return;

    uint8_t pTable  = (ppu->control.SPRITE_PATTERN_ADDR) ? 1 : 0;

	for (int i = 0; i < sizeof (ppu->OAMdata); i += 4) 
	{
		uint8_t xPos       = ppu->OAMdata[i + 3];
		uint8_t yPos       = ppu->OAMdata[i];
        uint8_t tile       = ppu->OAMdata[i + 1];
        uint8_t attributes = ppu->OAMdata[i + 2];

		if (yPos > y + 8) continue;

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
				uint8_t row1 = (Vflip) ? 7 - row + 1 : row + 1;
				ppu_pixel (ppu, xPos + col1, yPos + row1, palColor);
			}
		}
	}
}

inline void ppu_nametable_fetch (PPU2C02 * const ppu)
{
	ppu->nextTile.index = ppu_read (ppu, 0x2000 | (ppu->VRam.reg & 0xfff));
}

inline void ppu_attribute_fetch (PPU2C02 * const ppu)
{
	/* To be implemented */	
}

inline void ppu_load_BG_shifters (PPU2C02 * const ppu)
{
	/* To be implemented */
}

inline void ppu_copy_X_scroll (PPU2C02 * const ppu)
{
	if (!ppu->mask.RENDER_BG && !ppu->mask.RENDER_SPRITES) return;

	/*  VRam bits: ---- -N-- ---X XXXX */
	ppu->VRam.nametableX = ppu->tmpVRam.nametableX;
	ppu->VRam.coarseX    = ppu->tmpVRam.coarseX;
}

inline void ppu_copy_Y_scroll (PPU2C02 * const ppu)
{
	if (!ppu->mask.RENDER_BG && !ppu->mask.RENDER_SPRITES) return;

	/*  VRam bits: -yyy N-YY YYY- ---- */
	ppu->VRam.fineY      = ppu->tmpVRam.fineY;
	ppu->VRam.nametableY = ppu->tmpVRam.nametableY;
	ppu->VRam.coarseY    = ppu->tmpVRam.coarseY;
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
	/* Shorthand assignment */
	const int16_t cycle    = ppu->cycle;
	const int16_t scanline = ppu->scanline;

	int8_t pre_render = (scanline == 0);
	int8_t render = (scanline >= 1 && scanline <= 240);

	/* General scan/cycle counting */
	ppu->clockCount++;

	/* Pre-render line */
	if (pre_render && cycle >= 280 && cycle < 305)
	{
		ppu_copy_Y_scroll(ppu);
	}

	if (render)
	{				
		if (scanline == 0 && cycle == 1) {
			ppu->status.VERTICAL_BLANK = 0;
		}

		if (cycle == 257) {
			ppu_copy_X_scroll(ppu);
		}

		/* Do more PPU reads (not used) */
		if (cycle == 337 || cycle == 339) {
			ppu->nextTile.index = ppu_read(ppu, 0x2000 | (ppu->VRam.reg & 0x0fff));
		}
	}

	if (scanline == 242 && cycle == 1)
	{
		/* Debug nametable memory */
		copy_nametable (ppu, 0);
		copy_nametable (ppu, 1);
		
		ppu->status.VERTICAL_BLANK = 1;
		if (ppu->control.ENABLE_NMI) 
			ppu->nmi = 1;

#ifndef PPU_PIXEL
		ppu_background (ppu, cycle, scanline);
#endif
		//ppu_sprites (ppu);
	}

#ifdef PPU_PIXEL
	if (cycle < 256 && scanline < 240) {
		ppu_background (ppu, cycle, scanline);
		if (cycle == 255)
			ppu_sprites (ppu, cycle, scanline);
	}
#endif
	ppu->cycle++;

	if (cycle > 341)
	{
		ppu->cycle = 0;
		ppu->scanline++;
	}

	if (scanline > 260)
	{
		ppu->scanline = 0;
		ppu->nmi = 0;
		ppu->frame++;

		if (ppu->frame % 2) ppu->cycle++;
	}
}

void copy_nametable (PPU2C02 * const ppu, uint8_t const i)
{
	/* Loop through nametable values (bg tiles) */
	for (uint16_t ntTile = 0; ntTile < 960; ntTile++)
	{
		uint16_t xPos = (ntTile % 32) * 8;
		uint16_t yPos = (ntTile / 32) * 8;
		uint8_t val = ppu->nameTables[(i * 0x400) + ntTile];// ppu_read(ppu, 0x2000 + (i * 0x400) + ntTile);

		for (uint16_t row = 0; row < 8; row++)
		{
			/* Loop through a single row of the bit planes and combine values */
			for (uint16_t col = 0; col < 8; col++)
			{
				uint16_t pX = xPos + col;
				uint16_t pY = yPos + row;

				/* Add to nametable, with a shade based on pixel value */
				ppu->nTableDebug[i][(pY * 256 + pX) * 3]     =
				ppu->nTableDebug[i][(pY * 256 + pX) * 3 + 1] =
				ppu->nTableDebug[i][(pY * 256 + pX) * 3 + 2] = val;
			}
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