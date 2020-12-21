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
"uniform sampler2D text;\n"
"uniform vec3 textColor;\n"

"vec3 applyVignette(vec3 color)\n"
"{\n"
"    vec2 position = (TexCoords.xy / 256.0) - vec2(0.5);\n"           
"    float dist = length(position);\n"

"    float radius = 0.5;\n"
"    float softness = 0.02;\n"
"    float vignette = smoothstep(radius, radius - softness, dist);\n"

"    color.rgb = color.rgb - (1.0 - vignette);\n"

"    return color;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint = vec3(113, 115, 126) / 127.0;\n"
"    float sampled = texture2D(text, TexCoords).r;\n"
"    vec3 vgColor = applyVignette(vec3(sampled));\n"
"    gl_FragColor = vec4(vec3(sampled) * tint, 1.0);\n"
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

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, 256, 240, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	texture_create (&ppu->pTableTexture);
	texture_init (ppu->pTableTexture, GL_CLAMP_TO_EDGE, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	/* Create palette texture */
	texture_create (&ppu->paletteTexture);
	texture_init (ppu->paletteTexture, GL_CLAMP_TO_EDGE, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 64, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);	

	glBindTexture (GL_TEXTURE_2D, 0);
}

void ppu_reset (PPU2C02 * const ppu, NESrom * const rom)
{
	ppu_create_image (ppu);
	ppu->control = 0;
	ppu->mask = 0;
	ppu->status = 0;

	ppu->scanline = 0;
	ppu->cycle = 0;
	ppu->frame = 0;
	ppu->hiByte = 1;
	ppu->dataBuffer = 0;
	ppu->VRam = ppu->tmpVRam = 0x0;

	/* Copy the first 8K of CHR data as needed for mapper 0 */

	ppu->romCHR    = rom->CHRdata.data;
	ppu->mirroring = rom->mirroring;

	if (ppu->romCHR && rom->mapperID == 0)
	{
        copy_pattern_table (ppu, 0);
        copy_pattern_table (ppu, 1);
	}
}

/* Helper read functions */

inline void ppu_vram_increment (PPU2C02 * const ppu)
{
	ppu->VRam += (ppu->control & VRAM_ADD_INCREMENT) ? 32 : 1;
}

inline void ppu_read_status (PPU2C02 * const ppu, uint8_t * data)
{
	/* Combine top 3 bits of register and bottom 3 bits of data buffer */ 
	*data = (ppu->status & 0xe0) | (ppu->dataBuffer & 0x1f);

	/* Clear vblank */
	ppu->status &= (~VERTICAL_BLANK);
	ppu->hiByte = 1;
}

inline void ppu_read_oam_data (PPU2C02 * const ppu, uint8_t * data)
{
	/* To be implemented */
}

inline void ppu_read_data (PPU2C02 * const ppu, uint8_t * data)
{
	/* Return data from previous data request (dummy read). Data buffer
	   is returned in the next request */
	*data = ppu->dataBuffer;
	ppu->dataBuffer = ppu_read (ppu, ppu->VRam);

	/* The exception is reading from palette data, return it immediately */
	if (ppu->VRam >= 0x3f00)
		*data = ppu->dataBuffer;

	if (ppu->scanline >= 0 && ppu->scanline < 240 && ppu->cycle < 256)
	{
		//IncrementCourseX();
		//IncrementY();
	}
	else {
		ppu_vram_increment (ppu);
	}
}

/* Helper write functions */

inline void ppu_write_control (PPU2C02 * const ppu, const uint8_t data)
{
	uint8_t oldNMI = ppu->control & GENERATE_VBLANK_NMI;
	ppu->control = data;
	if (!oldNMI && (ppu->control & GENERATE_VBLANK_NMI) && (ppu->status & VERTICAL_BLANK)) 
	{
		printf("New vblank NMI\n");
		ppu->nmi = 1;
	}
}

inline void ppu_write_address (PPU2C02 * const ppu, const uint8_t data)
{
	if (ppu->hiByte == 1)
		ppu->tmpVRam = (uint16_t)((data & 0x3f) << 8) | (ppu->tmpVRam & 0x00ff);
	else {
		ppu->tmpVRam = (ppu->tmpVRam & 0xff00) | data;
		ppu->VRam = ppu->tmpVRam;
	}

	ppu->hiByte = 1 - ppu->hiByte;
}

inline void ppu_write_data (PPU2C02 * const ppu, const uint8_t data)
{
	ppu_write (ppu, ppu->VRam, data);
	ppu_vram_increment (ppu);
}

uint8_t ppu_register_read (PPU2C02 * const ppu, uint16_t const address)
{
	assert (address <= 0x7);
	uint8_t data = 0x00;
	
	switch (address)
	{	
		case PPU_STATUS: ppu_read_status   (ppu, &data); break;
		case OAM_DATA:   ppu_read_oam_data (ppu, &data); break;
		case PPU_DATA: 	 ppu_read_data     (ppu, &data); break;

		/* Unused */
		default:
			/* 0x0 (control), 0x1 (mask), 0x5 (scroll), 0x6 (PPU address) are write-only */
			break;
	}
	return data;
}

void ppu_register_write (PPU2C02 * const ppu, uint16_t const address, uint8_t const data)
{
	assert (address <= 0x7);

	switch (address)
	{
		case PPU_CONTROL: ppu_write_control (ppu, data); break;
		case PPU_MASK:    ppu->mask = data;              break;
		case PPU_ADDRESS: ppu_write_address (ppu, data); break;
		case PPU_DATA:	  ppu_write_data    (ppu, data); break;

		default:
			/* 0x2 (status) not usable here */
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
		data = ppu->romCHR[address];
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{
		/* Read from name table data (mirrored every 4KB) 
		   Todo: handle vertical mirroring mode */
		address &= 0x0fff;
		if (ppu->mirroring == MIRROR_HORIZONTAL)
		{
			// Horizontal
			if (address >= 0 && address <= 0x03ff)
				data = ppu->nameTables[address & 0x3ff];
			if (address >= 0x400 && address <= 0x7ff)
				data = ppu->nameTables[address & 0x3ff];
			if (address >= 0x800 && address <= 0xbff)
				data = ppu->nameTables[(address & 0x3ff) + 0x400];
			if (address >= 0xc00 && address <= 0xfff)
				data = ppu->nameTables[(address & 0x3ff) + 0x400];
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
		data = (ppu->paletteTable[address] & (ppu->mask & GRAYSCALE) ? 0x30 : 0x3f);
	}

	return data;
}

void ppu_write (PPU2C02 * const ppu, uint16_t address, uint8_t const data)
{
	if (address >= 0x0000 && address <= 0x1fff)
	{
		/* Write to CHR pattern table */
		//ppu->patternTables[address] = data;
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{
		/* Read from name table data (mirrored every 4KB) 
		   Todo: handle vertical mirroring mode */
		address &= 0x0fff;
		if (ppu->mirroring == MIRROR_HORIZONTAL)
		{
			// Horizontal
			if (address >= 0 && address <= 0x3ff)
				ppu->nameTables[address & 0x3ff] = data;
			if (address >= 0x400 && address <= 0x7ff)
				ppu->nameTables[address & 0x3ff] = data;
			if (address >= 0x800 && address <= 0xbff)
				ppu->nameTables[(address & 0x3ff) + 0x400] = data;
			if (address >= 0xc00 && address <= 0xfff)
				ppu->nameTables[(address & 0x3ff) + 0x400] = data;
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
		ppu->paletteTable[address] = data;
	}
}

void ppu_set_pixel (PPU2C02 * const ppu)
{

}

void ppu_clock (PPU2C02 * const ppu)
{
	ppu->clockCount++;
	ppu->cycle++;

	if (ppu->cycle >= 341)
	{
		ppu->cycle = 0;
		ppu->scanline++;

		if (ppu->scanline == 0)
		{
			//ppu->status &= (~VERTICAL_BLANK);
		}

		if (ppu->scanline == 241) /* Post visible scanline */
		{
			ppu->status |= VERTICAL_BLANK;
			ppu->status &= (~SPRITE_ZERO_HIT);

			/* Trigger VMI interrupt if needed */
			if (ppu->control & GENERATE_VBLANK_NMI) {
               	ppu->nmi = 1;
            }
			/* Debug rendering */
			ppu_render_bg (ppu);
		}

		if (ppu->scanline >= 262) /* 262 lines counting line 0 */
		{
			ppu->status &= (~VERTICAL_BLANK);
			ppu->status &= (~SPRITE_ZERO_HIT);
			ppu->scanline = 0;
			ppu->nmi = 0;
			ppu->frame++;

			//printf("New frame %d\n", ppu->frame);
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
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RED, 256, 240, 0, GL_RED, GL_UNSIGNED_BYTE, ppu->frameBuffer);
	draw_lazy_quad();

    /* Draw pattern tables */
    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 768, 0, 0);
    mat4x4_scale_aniso (model, model, 256, 256, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(ppu->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, ppu->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, ppu->patternTables[0]);
	draw_lazy_quad();

    mat4x4_identity (model);
    mat4x4_translate_in_place (model, 1024, 0, 0);
    mat4x4_scale_aniso (model, model, 256, 256, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(ppu->fbufferShader.program, "model"), 1, GL_FALSE, (const GLfloat*) model);

    glBindTexture (GL_TEXTURE_2D, ppu->pTableTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, ppu->patternTables[1]);
	draw_lazy_quad();

	/* Finish */
    glBindTexture(GL_TEXTURE_2D, 0);
}

void copy_pattern_table (PPU2C02 * const ppu, uint8_t const i) 
{
	for (uint16_t tile = 0; tile < 256; tile++)
	{
		/* Get offset value in memory based on tile position */
		uint16_t offset = i * 0x1000 + ((tile / 16) << 8) + ((tile & 15) << 4);

		/* Loop through each row of a tile */
		for (uint16_t row = 0; row < 8; row++)
		{
			uint8_t tile_lsb = ppu->romCHR [offset + row];
			uint8_t tile_msb = ppu->romCHR [offset + row + 8];

			/* Loop through a single row of the bit planes and combine values */
			for (uint16_t col = 0; col < 8; col++)
			{
				/* Get rightmost bit from each byte in memory */
				uint8_t pixel = (tile_lsb & 1) + ((tile_msb & 1) << 1);

				/* X axis needs to be flipped */
				uint16_t pX = ((tile & 0xf) << 3) + (7 - col);
				uint16_t pY = ((tile >> 4) << 3)  + row;

				/* Add to pattern table, with a shade based on pixel value */
				ppu->patternTables[i][pY * 128 + pX] = pixel * 0x55;

				tile_lsb >>= 1;
				tile_msb >>= 1;
			}
		}
	}
}

void ppu_render_bg (PPU2C02 * const ppu)
{
	uint16_t table = (ppu->control & BACKROUND_PATTERN_ADDR) ? 1 : 0;

	/* Loop through 960 entries in nametable */
	for (int y = 0; y < 30; y++) 
	{
		//printf ("Ntable ");
		for (int x = 0; x < 32; x++) 
		{
			/* Get offset value in memory based on tile position */
			uint8_t tile = ppu->nameTables[(y * 32) + x];
			uint8_t xPos = x * 8;
			uint8_t yPos = y * 8;

			/* Positioning in pattern table */
			uint16_t pX = (tile & 15) << 3;
			uint16_t pY = (tile >> 4) << 3;

			for (uint16_t row = 0; row < 8; row++)
			{
				/* Loop through a single row of the bit planes and combine values */
				for (uint16_t col = 0; col < 8; col++)
				{
					ppu->frameBuffer[((yPos + row) * 256) + (xPos + col)] = 
						ppu->patternTables[table][((pY + row) * 128) + pX + col];
				}
			}
			//printf("%02x ", tile);
		}
		//printf("\n");
	}
}