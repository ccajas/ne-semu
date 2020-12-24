#include "text.h"
#include "texture.h"

const char* text_vs_source =

"#version 330 core\n"
"layout (location = 0) in vec4 vertex;\n"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{\n"
"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
"    TexCoords = vertex.zw;\n"
"}\n";
 
const char* text_fs_source =

"#version 330 core\n"
"in vec2 TexCoords;\n"
"out vec4 color;\n"
"uniform sampler2D text;\n"
"uniform vec4 textColor;\n"

"void main()\n"
"{\n"
"    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
"    if (texture(text, TexCoords).r < 0.02)\n"
"        discard;\n"
"    color = vec4(textColor.rgb, sampled * textColor.a) * sampled;\n"
"}\n";

void text_init()
{
    FT_Library ft;
    int error = FT_Init_FreeType(&ft);
    if (error) {
        e_printf(error, "ERROR::FREETYTPE: Could not init FreeType Library \n");
    }
    else {
        printf("Init freetype \n");
    }

    const char* homeDir = getenv("HOME");
    char textbuf[256] = "";
    sprintf(textbuf, "%s/.local/share/fonts/monogram.ttf", homeDir);

    char fontStack[][256] = {
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/OpenSans-Regular.ttf",
        "/usr/share/fonts/opentype/firacode/FiraCode-Regular.otf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf"
    };

    FT_Face face;
    int lastFont = 0;

    /* Attempt to load a font and use the first one that works */
    for (int i = 0; i < 4; i++)
    {
        error = FT_New_Face(ft, fontStack[i], 0, &face);
        if (!error) { 
            break; 
        }
        else {
            lastFont = i;
        }
    }

    if (error) {
        printf("ERROR::FREETYTPE: %d: Failed to load font '%s' \n", error, fontStack[lastFont]);
    }

    FT_Set_Pixel_Sizes(face, 0, CHAR_PIXEL_SIZE);

    /* Create shader */
    textShader = shader_init_source (text_vs_source, text_fs_source);

    /* Disable byte-alignment restriction */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint texture;
    texture_create(&texture);
    texture_init (texture, GL_CLAMP_TO_EDGE, GL_LINEAR);

    int max_dim = (1 + (face->size->metrics.height >> 6)) * ceilf(sqrtf(TOTAL_CHARS));
	int tex_width = 1;
	while(tex_width < max_dim) tex_width <<= 1;
	int tex_height = tex_width;

	/* render glyphs to atlas */
	
	char* pixels = (char*)calloc(tex_width * tex_height, 1);
	float pen_x = 0, pen_y = 0;

    for (int c = 0; c < TOTAL_CHARS; c++)
    {
        /* Load each character glyph */
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("ERROR::FREETYTPE: Failed to load Glyph %d\n", c);
            continue;
        }

		if (pen_x + face->glyph->bitmap.width >= tex_width)
        {
			pen_x = 0;
			pen_y += ((face->size->metrics.height >> 6) + 1);
		}

		for (int row = 0; row < face->glyph->bitmap.rows; row++)
        {
			for (int col = 0; col < face->glyph->bitmap.width; col++)
            {
				int x = pen_x + col;
				int y = pen_y + row;
				pixels[y * tex_width + x] = face->glyph->bitmap.buffer[row * face->glyph->bitmap.pitch + col];
			}
		}

        /* Store character for later use */
        struct Character character = {
            texture,
            pen_x, pen_y,
            { face->glyph->bitmap.width, face->glyph->bitmap.rows },
            { face->glyph->bitmap_left, face->glyph->bitmap_top },
            face->glyph->advance.x
        };
        characters[c] = character;

        pen_x += face->glyph->bitmap.width + 16;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width, tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(pixels);

    /* Clean up after using FreeType */
    
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    /* Configure textVAO/VBO for textured quads */
    
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void text_begin(int scrWidth, int scrHeight)
{
    glUseProgram(textShader.program);
    mat4x4 p;
    mat4x4_ortho(p, 0, scrWidth, 0, scrHeight, 0, 1.0f);
    glUniformMatrix4fv (glGetUniformLocation(textShader.program, "projection"), 1, GL_FALSE, (const GLfloat*) p);
}

void text_draw_color (
    const char text[], 
    float x, 
    float y,
    float scale, 
    vec4 color)
{
    glUniform4f(glGetUniformLocation(textShader.program, "textColor"), color[0], color[1], color[2], color[3]);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // Iterate through all characters
    int len = strlen(text);

    /* Set texture */
    glBindTexture(GL_TEXTURE_2D, characters[0].TextureID);

    for (int i = 0; i < len; i++) 
    {
        struct Character ch = characters[(int)text[i]];

        GLfloat xpos = x + ch.bearing[0] * scale;
        GLfloat ypos = y - (ch.size[1] - ch.bearing[1]) * scale;

        GLfloat w = ch.size[0] * scale;
        GLfloat h = ch.size[1] * scale;

        GLfloat u = ch.px / 512;
        GLfloat v = ch.py / 512;
        GLfloat w1 = ch.size[0] / 512;
        GLfloat h1 = ch.size[1] / 512;

        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   u, v },            
            { xpos,     ypos,       u, v + h1 },
            { xpos + w, ypos,       u + w1, v + h1 },

            { xpos,     ypos + h,   u, v },
            { xpos + w, ypos,       u + w1, v + h1 },
            { xpos + w, ypos + h,   u + w1, v }           
        };

        /* Begin rendering quad */

        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        /* Advance position for next glyph (note that advance is number of 1/64 pixels so shift by 6 bits) */
        x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
