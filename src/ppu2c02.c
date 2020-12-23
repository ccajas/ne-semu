#include "ppu2c02.h"
#include "rom.h"
#include "gfx/texture.h"

const char * ppu_vs_source =

"#version 330\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
"layout (location = 1) in vec2 texture;"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"

"void main()\n"
"{\n"
"    vec4 localPos = model * vec4(vertex.xyz, 1.0);"
"    gl_Position = projection * vec4(localPos.xy, 0.0, 1.0);\n"
"    TexCoords = texture.xy;\n"
"}\n";
 
const char * ppu_fs_source =

"#version 110\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"

"vec3 applyVignette(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy) - vec2(0.5);\n"           
"    float dist = length(position);\n"

"    float radius = 1.8;\n"
"    float softness = 1.0;\n"
"    float vignette = smoothstep(radius, radius - softness, dist);\n"
"    color.rgb = color.rgb - (0.95 - vignette);\n"
"    return color;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint    = vec3(113, 115, 126) / 127.0;\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    vec4 texel   = texture2D(colorPalette, vec2(sampled.r / 4.0, 0.0));\n"
"    gl_FragColor = vec4(applyVignette(sampled.rgb), 1.0);\n"
"}\n";

const uint16_t palette2C03[64] = 
{
    0x333, 0x014, 0x006, 0x326, 0x403, 0x503, 0x510, 0x420, 0x320, 0x120, 0x031, 0x040, 0x022, 0,     0, 0,
    0x555, 0x036, 0x027, 0x407, 0x507, 0x704, 0x700, 0x630, 0x430, 0x140, 0x040, 0x053, 0x044, 0,     0, 0,
    0x777, 0x357, 0x447, 0x637, 0x707, 0x737, 0x740, 0x750, 0x660, 0x360, 0x070, 0x276, 0x077, 0x222, 0, 0,
    0x777, 0x567, 0x657, 0x757, 0x747, 0x755, 0x764, 0x772, 0x773, 0x572, 0x473, 0x276, 0x467, 0x555, 0, 0
};

void ppu_create_image (PPU2C02 * const ppu)
{
	/* Create shader */
	ppu->fbufferShader = shader_init_source (ppu_vs_source, ppu_fs_source);

	/* Disable byte-alignment restriction */
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Create textures */
	texture_create (&ppu->fbufferTexture);
	texture_init (ppu->fbufferTexture, GL_CLAMP_TO_EDGE, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	texture_create (&ppu->pTableTexture);
	texture_init (ppu->pTableTexture, GL_CLAMP_TO_EDGE, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	/* Create palette texture */
	texture_create (&ppu->paletteTexture);
	texture_init (ppu->paletteTexture, GL_CLAMP_TO_EDGE, GL_NEAREST);

	char* pixels = (char*)calloc(192, 1);
	for (int i = 0; i < 192; i += 3)
	{
		uint16_t palColor = palette2C03[i / 3];
		pixels[i] =   (palColor >> 8) << 5;
		pixels[i+1] = (palColor >> 4) << 5;
		pixels[i+2] = (palColor << 5);
	}

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 64, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);	
	free (pixels);
}

void ppu_reset (PPU2C02 * const ppu, NESrom * const rom)
{
	ppu_create_image (ppu);
	ppu->control.flags = 0;
	ppu->mask.flags = 0;
	ppu->status.flags = 0;

	ppu->scanline = 261;
	ppu->cycle = ppu->frame = 0;
	ppu->latch = 0;
	ppu->fineX = 0;
	ppu->dataBuffer = 0;

	ppu->bg_shifter_pattern_lo = ppu->bg_shifter_pattern_hi = 0;
	ppu->VRam.reg = ppu->tmpVRam.reg = 0x0;

	memset(&ppu->patternTables[0], 0, 4096);
	memset(&ppu->patternTables[1], 0, 4096);

	memset(&ppu->nameTables[0], 0, 1024);
	memset(&ppu->nameTables[1], 0, 1024);

	/* Copy the first 8K of CHR data as needed for mapper 0 */

	ppu->romCHR    = rom->CHRdata.data;
	ppu->mirroring = rom->mirroring;

	if (ppu->romCHR && rom->mapperID == 0)
	{
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
			break; /*Todo */
		case PPU_DATA:
			/* Reads here are delayed, retrieve from the buffer */
			data = ppu->dataBuffer;
			ppu->dataBuffer = ppu_read(ppu, ppu->VRam.reg);

			/* Fetch immediately if address is palette data */
			if (ppu->VRam.reg >= 0x3f00) 
				data = ppu->dataBuffer;

			if (ppu->control.VRAM_ADD_INCREMENT)
				printf("VRAM add increment\n");

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
	const uint16_t POWERUP_CYCLES = 29658;

	/* Some write registers are not ready during power up time */
	switch (address)
	{
		case PPU_CONTROL: case PPU_MASK:
		case PPU_SCROLL:  case PPU_ADDRESS:
			if (ppu->clockCount < POWERUP_CYCLES) return;
	}

	switch (address)
	{
		case PPU_CONTROL:
			ppu->control.flags = data;
			ppu->tmpVRam.nametableY = ppu->control.NAMETABLE_1;
			ppu->tmpVRam.nametableY = ppu->control.NAMETABLE_2;
			break;
		case PPU_MASK:
			ppu->mask.flags = data;
			break;
		case OAM_ADDRESS:
			ppu->OAMaddress = data;
			break;
		case OAM_DATA:
			break; /* Todo */
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
				ppu->tmpVRam.reg = (uint16_t)((data & 0x3f) << 8) + (ppu->tmpVRam.reg & 0xff);
			}
			else
			{
				ppu->tmpVRam.reg = (ppu->tmpVRam.reg & 0xff00) + data;
				ppu->VRam = ppu->tmpVRam;
			}
			ppu->latch = ~ppu->latch;
			break;
		case PPU_DATA:
			ppu_write (ppu, ppu->VRam.reg, data);

			if (ppu->control.VRAM_ADD_INCREMENT)
				printf("Frame %d: VRAM add increment from write $2007 \n", ppu->frame);

			ppu->VRam.reg += (ppu->control.VRAM_ADD_INCREMENT) ? 32 : 1;
			break;
		default: /* PPU status ($2002) not writable */
			break;
	}
}

uint8_t ppu_read (PPU2C02 * const ppu, uint16_t address)
{
	/* Address should be mapped to lowest 16KB */
	assert (address <= 0x3fff);
	uint8_t data = 0x00;

	if (address >= 0x0000 && address <= 0x1fff)
	{
		/* Read from CHR pattern table */
		//printf("Read from $%04x\n", address);
		data = ppu->patternTables[address >> 12][address & 0xfff];//ppu->romCHR[address];
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{
		/* Read from nametable data (mirrored every 4KB) */
		address &= 0x0fff;
		if (ppu->mirroring == MIRROR_HORIZONTAL)
		{
			if (address >= 0 && address <= 0x03ff)
				data = ppu->nameTables [0][address & 0x3ff];
			if (address >= 0x400 && address <= 0x7ff)
				data = ppu->nameTables [0][address & 0x3ff];
			if (address >= 0x800 && address <= 0xbff)
				data = ppu->nameTables [1][address & 0x3ff];
			if (address >= 0xc00 && address <= 0xfff)
				data = ppu->nameTables [1][address & 0x3ff];
		}
		if (ppu->mirroring == MIRROR_VERTICAL)
		{
			if (address >= 0 && address <= 0x03ff)
				data = ppu->nameTables [0][address & 0x3ff];
			if (address >= 0x400 && address <= 0x7ff)
				data = ppu->nameTables [1][address & 0x3ff];
			if (address >= 0x800 && address <= 0xbff)
				data = ppu->nameTables [0][address & 0x3ff];
			if (address >= 0xc00 && address <= 0xfff)
				data = ppu->nameTables [1][address & 0x3ff];
		}
	}
	else if (address >= 0x3f00 && address <= 0x3fff)
	{
		/* Read from palette table data */
		address &= 0x1f;
		if (address == 0x10) address = 0x0;
		if (address == 0x14) address = 0x4;
		if (address == 0x18) address = 0x8;
		if (address == 0x1c) address = 0xc;
		data = ppu->paletteTable[address];
	}

	return data;
}

void ppu_write (PPU2C02 * const ppu, uint16_t address, uint8_t const data)
{
	if (address >= 0 && address <= 0x1fff)
	{
		/* Write to CHR pattern table */
		printf("Frame %d: Write attempt to CHR ROM: addr %04x data %02x cycle %d\n", ppu->frame, address, data, ppu->cycle);		
		ppu->patternTables [(address & 0x1000) >> 12][address & 0xfff] = data;
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{
		/* Write to nametable data (mirrored every 4KB) */
		address &= 0x0fff;
		if (ppu->mirroring == MIRROR_HORIZONTAL)
		{
			if (address >= 0 && address <= 0x3ff)
				ppu->nameTables [0][address & 0x3ff] = data;
				
			if (address >= 0x400 && address <= 0x7ff)
				ppu->nameTables [0][address & 0x3ff] = data;

			if (address >= 0x800 && address <= 0xbff)
				ppu->nameTables [1][address & 0x3ff] = data;

			if (address >= 0xc00 && address <= 0xfff)
				ppu->nameTables [1][address & 0x3ff] = data;

		}
		else if (ppu->mirroring == MIRROR_VERTICAL)
		{
			if (address >= 0 && address <= 0x3ff)
				ppu->nameTables [0][address & 0x3ff] = data;

			if (address >= 0x400 && address <= 0x7ff)
				ppu->nameTables [1][address & 0x3ff] = data;

			if (address >= 0x800 && address <= 0xbff)
				ppu->nameTables [0][address & 0x3ff] = data;
				
			if (address >= 0xc00 && address <= 0xfff)
				ppu->nameTables [1][address & 0x3ff] = data;
		}
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

void ppu_set_pixel (PPU2C02 * const ppu, uint16_t const x, uint16_t const y)
{
	uint16_t tileX = x / 8;
	uint16_t tileY = y / 8;
	uint16_t row = x % 8;
	uint16_t col = y % 8;

	uint16_t pTable = (ppu->control.BACKGROUND_PATTERN_ADDR) ? 1 : 0;

	/* Get offset value in memory based on tile position */
	uint8_t baseTable = ppu->control.NAMETABLE_1 | ppu->control.NAMETABLE_2;
	uint8_t tile = ppu_read(ppu, (tileY * 32 + tileX) + (0x2000 | baseTable * 0x400));
	uint8_t xPos = tileX * 8;
	uint8_t yPos = tileY * 8;

	/* Get attribute table info */
	uint16_t attrTableIndex = ((tileY / 4) * 8) + tileX / 4;
	uint8_t  attrByte = ppu_read(ppu, 0x23c0 + attrTableIndex);
	uint8_t  palette  = attrByte >> ((tileY % 4 / 2) << 2 | (tileX % 4 / 2) << 1) & 3;

	/* Combine bitplanes and color the pixel */
	uint16_t offset   = (pTable << 12) + (uint16_t)(tile << 4);
	uint8_t  tile_lsb = ppu_read(ppu, offset + row + 8) >> (7 - col);
	uint8_t  tile_msb = ppu_read(ppu, offset + row) >> (7 - col);
	uint8_t  index    = (tile_msb & 1) + ((tile_lsb & 1) << 1);

	uint16_t palColor = palette2C03[ppu_read(ppu, 0x3f00 + (palette << 2) + index) & 0x3f];
	uint16_t p = ((yPos + row) << 8) + (xPos + col);

	ppu->frameBuffer[p * 3]   = (uint8_t)(palColor >> 8) << 5;
	ppu->frameBuffer[p * 3+1] = (uint8_t)(palColor >> 4) << 5;
	ppu->frameBuffer[p * 3+2] = (uint8_t)(palColor << 5);
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
	uint16_t cycleStart = ppu->cycle == 1;
	/*uint16_t visible    = ppu->scanline >= 0 && ppu->scanline < 240;
	uint16_t preLine    = ppu->scanline == 261;*/
	uint16_t prefetch     = ppu->cycle >= 321 && ppu->cycle <= 336;
	uint16_t visibleFetch = ppu->cycle >= 1   && ppu->cycle <= 256;
	uint16_t fetchCycle   = prefetch || visibleFetch;

	if ((ppu->scanline >= 0 && ppu->scanline < 240) || ppu->scanline == 261)
	{		
		if (ppu->scanline == 0 && ppu->cycle == 0)
		{
			// "Odd Frame" cycle skip
			if (ppu->frame & 1) ppu->cycle = 1;
		}
		

		if (ppu->scanline == 261 && cycleStart)
		{
			// Effectively start of new frame, so clear vertical blank flag
			ppu->status.VERTICAL_BLANK = 0;
			if (ppu->control.ENABLE_NMI) 
				ppu->nmi = 1;
		}

		/* Do more PPU reads (not used) */
		if (ppu->cycle == 337 || ppu->cycle == 339)
		{
			ppu->nextTile.index = ppu_read(ppu, 0x2000 | (ppu->VRam.reg & 0x0fff));
		}
	}

	if (ppu->scanline >= 241 && ppu->scanline < 261)
	{
		if (ppu->scanline == 241 && cycleStart)
		{
			ppu->status.VERTICAL_BLANK = 1;
			//ppu_set_bg (ppu);
		}
	}

	if (ppu->cycle < 256 && ppu->scanline < 240)
	{
		ppu_set_pixel (ppu, ppu->cycle, ppu->scanline);
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
			ppu->frame++;
		}
	}
}

uint32_t quadVAO = 0;
uint32_t quadVBO = 0;

float quadVertices[] = {

     0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
     1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
     1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
};

void draw_lazy_quad()
{
    if (quadVAO == 0)
    {
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 
    }

    glBindVertexArray(quadVAO);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void ppu_debug (PPU2C02 * const ppu, int32_t const scrWidth, int32_t const scrHeight)
{
	uint16_t w = 768, h = 720;

    mat4x4 model;
    mat4x4_identity (model);
    mat4x4_scale_aniso (model, model, w, h, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(ppu->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glActiveTexture (GL_TEXTURE0);

    /* Draw framebuffer */
    glBindTexture (GL_TEXTURE_2D, ppu->fbufferTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, ppu->frameBuffer);
	draw_lazy_quad();

    /* Draw pattern tables */
    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 768, 0, 0);
    mat4x4_scale_aniso (model, model, 256, 256, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(ppu->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, ppu->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, ppu->pTableDebug[0]);
	draw_lazy_quad();

    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 1024, 0, 0);
    mat4x4_scale_aniso (model, model, 256, 256, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(ppu->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, ppu->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, ppu->pTableDebug[1]);

    //glBindTexture (GL_TEXTURE_2D, ppu->paletteTexture);
    //glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, ppu->fullPixels);

	draw_lazy_quad();

	/* Bind the palette to the 2nd texture too */
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ppu->paletteTexture);

	/* Finish */
    glBindTexture(GL_TEXTURE_2D, 0);
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
			uint8_t tile_lsb = ppu->romCHR [offset + row];
			uint8_t tile_msb = ppu->romCHR [offset + row + 8];

			/* Copy into pattern tables first */
			ppu->patternTables[i][(offset + row)     & 0xfff] = tile_lsb;
			ppu->patternTables[i][(offset + row + 8) & 0xfff] = tile_msb;

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

void nametable_debug (PPU2C02 * const ppu, const int index)
{
	char textbuf[256];
	for (int i = 0; i < 0x3c0; i++)
	{
		if (i % 32 == 0) {
			sprintf(textbuf, " ");
		}
		sprintf(textbuf + strlen(textbuf), "%02x ", ppu->nameTables[index][i & 0x3ff]);
		if (i % 32 == 31) {
			printf("%s\n", textbuf);
		}
	}
	printf("...\n");
}