#include "ppu2c02.h"
#include "rom.h"
#include "gfx/texture.h"

const char * ppu_vs_source =

"#version 330 core\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{\n"
"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
"    TexCoords = vertex.zw;\n"
"}\n";
 
const char * ppu_fs_source =

"#version 330 core\n"
"in vec2 TexCoords;\n"
"out vec4 color;\n"
"uniform sampler2D text;\n"
"uniform vec3 textColor;\n"

"void main()\n"
"{\n"
"    float sampled = texture(text, TexCoords).r;\n"
"    color = vec4(vec3(sampled), 1.0);\n"
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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    texture_create (&ppu->texture);
    texture_init (ppu->texture, GL_CLAMP_TO_EDGE, GL_NEAREST);

	const uint8_t w = 128, h = 128;

	char* pixels = (char*)calloc(w * h, 1);
	for (int row = 0; row < h; row++)
	{
		for (int col = 0; col < w; col++)
		{
			pixels[row * w + col] = random() & 0xff;
		}
	}

    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glBindTexture (GL_TEXTURE_2D, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    free(pixels);

    /* Configure VAO and VBO for textured quads */
    
    glGenVertexArrays(1, &ppu->fbufferVAO);
    glGenBuffers(1, &ppu->fbufferVBO);
    glBindVertexArray(ppu->fbufferVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ppu->fbufferVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ppu_reset (PPU2C02 * const ppu, NESrom * const rom)
{
	ppu_create_image (ppu);
	ppu->scanline = 0;
	ppu->cycle = 0;
	ppu->frame = 0;

	/* Give direct access to rom's CHR data via pointer (read/write) */
	ppu->romCHR = rom->CHRdata.data;

	if (ppu->romCHR)
	{
        dump_pattern_table (ppu, 0);
        dump_pattern_table (ppu, 1);
	}
}

void ppu_vram_increment (PPU2C02 * const ppu)
{
	ppu->VramAddress += (ppu->control & 0x4) ? 32 : 1;
}

uint8_t ppu_cpu_read (PPU2C02 * const ppu, uint16_t const address)
{
	assert (address <= 0x7);
	uint8_t data = 0x00;
	
	switch (address)
	{	
		case PPU_STATUS:
			/* Combine top 3 bits of register and bottom 3 bits of data buffer */ 
			data = (ppu->status & 0xe0) | (ppu->dataBuffer & 0x1f);

			/* Clear vblank */
			ppu->status &= (~VERTICAL_BLANK);
			ppu->addressLatch = 0;
			break;

		case PPU_DATA:

			/* Return data from previous data request (dummy read). Data buffer
			   is returned in the next request */
			data = ppu->dataBuffer;
			ppu->dataBuffer = ppu_read (ppu, ppu->VramAddress);

			/* The exception is reading from palette data, return it immediately */
			if (ppu->VramAddress >= 0x3f00)
				data = ppu->dataBuffer;

			ppu_vram_increment (ppu);
			break;

		/* Unused */
		default:
			/* 0x0 (control), 0x1 (mask), 0x5 (scroll), 0x6 (PPU address) are write-only */
			break;
	}
	return data;
}

void ppu_cpu_write (PPU2C02 * const ppu, uint16_t const address, uint8_t const data)
{
	assert (address <= 0x7);

	switch (address)
	{
		case PPU_CONTROL:
			/* Just write to control register */
			ppu->control = data;
			break;

		case PPU_ADDRESS:

			break;

		case PPU_DATA:
			ppu_write (ppu, ppu->VramAddress, data);
			ppu_vram_increment (ppu);
			break;

		default:
			/* 0x2 (status) not usable here */
			break;
	}
}

uint8_t ppu_read (PPU2C02 * const ppu, uint16_t address)
{
	/* Address should be mapped to lowest 16kB */
	assert (address <= 0x3fff);
	uint8_t data = 0x00;

	if (address >= 0x0000 && address <= 0x1fff)
	{
		data = ppu->romCHR[address];
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{
		/* Read from name table data (mirrored every 4KB) 
		   Todo: handle vertical mirroring mode */
		address &= 0x0fff;
		data = ppu->nameTables[address];
	}
	else if (address >= 0x3f00 && address <= 0x3fff)
	{
		address &= 0x001f;
		if (address == 0x0010) address = 0x0000;
		if (address == 0x0014) address = 0x0004;
		if (address == 0x0018) address = 0x0008;
		if (address == 0x001C) address = 0x000C;
		//data = tblPalette[address] & (mask.grayscale ? 0x30 : 0x3F);
	}

	return data;
}

void ppu_write (PPU2C02 * const ppu, uint16_t const address, uint8_t const data)
{

}

void ppu_clock (PPU2C02 * const ppu)
{
	/* render to FBO
	glBindFramebuffer( GL_FRAMEBUFFER, FFrameBuffer );
	glViewport( 0, 0, 256, 240);

	glBindFramebuffer( GL_FRAMEBUFFER, 0); */

	ppu->clockCount++;
	ppu->cycle++;

	if (ppu->cycle >= 341)
	{
		ppu->cycle = 0;
		ppu->scanline++;

		if (ppu->scanline >= 1 && ppu->scanline <= 240) /* Visible scanlines */
		{
			/* Should scan through OAM here. Also, set pixels */
		}

		if (ppu->scanline == 241) /* Post visible scanline */
		{
			/* Trigger VMI interrupt if needed */
			if (ppu->control & GENERATE_VBLANK_NMI)
			{
               	ppu->status |= VERTICAL_BLANK;
	
            }
		}

		if (ppu->scanline == 261) /* Pre-render scanline */
		{

		}

		if (ppu->scanline >= 262) /* 262 lines counting line 0 */
		{
			ppu->status &= (~VERTICAL_BLANK);
			ppu->scanline = 0;
			ppu->frame++;
		}
	}
}

void ppu_debug (PPU2C02 * const ppu, int32_t const scrWidth, int32_t const scrHeight, uint8_t const idx)
{
	/* Test draw, use PPU shader */
    glUseProgram(ppu->fbufferShader.program);
    mat4x4 p;
    mat4x4_ortho(p, 0, scrWidth, 0, scrHeight, 0, 0.1f);
    glUniformMatrix4fv (glGetUniformLocation(ppu->fbufferShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) p);

    glUniform3f(glGetUniformLocation(ppu->fbufferShader.program, "textColor"), 0.0, 0.0, 0.0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(ppu->fbufferVAO);

    /* Set texture */
    glBindTexture(GL_TEXTURE_2D, ppu->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Generate new random noise */
	uint16_t w = 256,  h = 256;
	/*char* pixels = (char*)calloc(128 * 128, 1);
	for (int row = 0; row < 128; row++)
	{
		for (int col = 0; col < 128; col++)
		{
			pixels[row * 128 + col] = random() & 0xff;
		}
	}*/
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 128, GL_RED, GL_UNSIGNED_BYTE, ppu->patternTable[idx]);
	//free (pixels);
	int16_t xpos = scrWidth - 512, ypos = 0;
	if (idx > 0) xpos += 256;

	GLfloat vertices[6][4] = {
		{ xpos,     ypos + h,   0.0, 0.0 },            
		{ xpos,     ypos,       0.0, 1.0 },
		{ xpos + w, ypos,       1.0, 1.0 },

		{ xpos,     ypos + h,   0.0, 0.0 },
		{ xpos + w, ypos,       1.0, 1.0 },
		{ xpos + w, ypos + h,   1.0, 0.0 }           
	};
	
	/* Render quad */
	glBindBuffer(GL_ARRAY_BUFFER, ppu->fbufferVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	/* Finish */
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void dump_pattern_table (PPU2C02 * const ppu, uint8_t const i) 
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
				ppu->patternTable[i][pY * 128 + pX] = pixel * 0x55;

				tile_lsb >>= 1;
				tile_msb >>= 1;
			}
		}
	}
}