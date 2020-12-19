#include "ppu2c02.h"
#include "rom.h"
#include "gfx/texture.h"

const char* ppu_vs_source =

"#version 330 core\n"
"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{\n"
"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
"    TexCoords = vertex.zw;\n"
"}\n";
 
const char* ppu_fs_source =

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

void ppu_reset (PPU2C02 * const ppu)
{
	ppu_create_image (ppu);
/*
	// setup FBO
	glGenFramebuffers( 1, &FFrameBuffer );
	glBindFramebuffer( GL_FRAMEBUFFER, FFrameBuffer );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0 );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	// cleanup
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glDeleteFramebuffers( 1, &FFrameBuffer );
*/
}

uint8_t ppu_cpu_read (PPU2C02 * const ppu, uint16_t const address)
{
	printf("PPU read!\n");
	uint8_t data = 0x00;
	/*switch (addr)
		{	
			// Status
		case 0x2:
			// Reading from the status register has the effect of resetting
			// different parts of the circuit. Only the top three bits
			// contain status information, however it is possible that
			// some "noise" gets picked up on the bottom 5 bits which 
			// represent the last PPU bus transaction. Some games "may"
			// use this noise as valid data (even though they probably
			// shouldn't)
			data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);

			// Clear the vertical blanking flag
			status.vertical_blank = 0;

			// Reset Loopy's Address latch flag
			address_latch = 0;
			break;

			// OAM Address
		case 0x0003: break;

			// OAM Data
		case 0x0004: break;

			// PPU Data
		case 0x0007: 
			data = ppu_data_buffer;
			ppu_data_buffer = ppuRead(vram_addr.reg);
			if (vram_addr.reg >= 0x3F00) data = ppu_data_buffer;
			vram_addr.reg += (control.increment_mode ? 32 : 1);
			break;
		default:
			break;
		}*/
	return data;
}

uint8_t ppu_read (PPU2C02 * const ppu, uint16_t address)
{
	/* Map to lowest 16kB */
	uint8_t data = 0x00;
	address &= 0x3fff;

	/*if (cart->ppuRead(addr, data))
	{

	}*/
	if (address >= 0x0000 && address <= 0x1fff)
	{
		data = ppu->patternTable[(address & 0x1000) >> 12][address & 0x0fff];
	}
	else if (address >= 0x2000 && address <= 0x3eff)
	{

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

void ppu_clock (PPU2C02 * const ppu)
{
	// Fake some noise for now
	//sprScreen.SetPixel(cycle - 1, scanline, palScreen[(rand() % 2) ? 0x3F : 0x30]);

	// render to FBO
	//glBindFramebuffer( GL_FRAMEBUFFER, FFrameBuffer );
	//glViewport( 0, 0, 256, 240);

	//glBindFramebuffer( GL_FRAMEBUFFER, 0);

	ppu->clockCount++;

	if (ppu->cycle >= 341)
	{
		ppu->cycle = 0;
		ppu->scanline++;
		if (ppu->scanline >= 261)
		{
			ppu->scanline = -1;
			//frame_complete = true;
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

void dump_pattern_table (PPU2C02 * const ppu, NESrom * const cart, uint8_t const i) 
{
	// Loop through all 16x16 tiles
	for (uint16_t nTileY = 0; nTileY < 16; nTileY++)
	{
		for (uint16_t nTileX = 0; nTileX < 16; nTileX++)
		{
			// Convert the 2D tile coordinate into a 1D offset into the pattern
			// table memory.
			uint16_t offset = i * 0x1000 + ((nTileY << 8) + (nTileX << 4));

			// Now loop through 8 rows of 8 pixels
			for (uint16_t row = 0; row < 8; row++)
			{
				// For each row, we need to read both bit planes of the character
				// in order to extract the least significant and most significant 
				// bits of the 2 bit pixel value. in the CHR ROM, each character
				// is stored as 64 bits of lsb, followed by 64 bits of msb. This
				// conveniently means that two corresponding rows are always 8
				// bytes apart in memory.
				uint8_t tile_lsb = cart->chr_data[offset + row];
				uint8_t tile_msb = cart->chr_data[offset + row + 8];

				// Now we have a single row of the two bit planes for the character
				// we need to iterate through the 8-bit words, combining them to give
				// us the final pixel index
				for (uint16_t col = 0; col < 8; col++)
				{
					/* Get rightmost bit from each byte in memory */
					uint8_t pixel = (tile_lsb & 1) + (tile_msb & 1);

					/* X axis needs to be flipped */
					uint16_t pX = (nTileX << 3) + (7 - col);
					uint16_t pY = (nTileY << 3) + row;

					/* Add to pattern table, with a shade based on pixel value */
					ppu->patternTable[i][pY * 128 + pX] = pixel * 0x55;
	
					tile_lsb >>= 1;
					tile_msb >>= 1;
				}
			}
		}
	}
}