#ifndef IMAGE_PARSERS_H
#define IMAGE_PARSERS_H

#include "../utils/filereaders.h"

static inline unsigned char* read_BMP(const char* filename, uint32_t* width, uint32_t* height)
{
    FILE* f = fopen(filename, "rb");
	if (f == NULL) {
        return NULL;
    }
    unsigned char info[54];
	/* Read header */
    if (!fread(info, sizeof(char), 54, f)) {
        return NULL;
    }

    /* extract image height and width from header */
    uint16_t bpp;
    memcpy(width, info + 18, sizeof(int));
    memcpy(height, info + 22, sizeof(int));
    memcpy(&bpp, info + 28, sizeof(uint16_t));
    
    /* extract image size from header */
    int imgSize = 0;
    memcpy(&imgSize, info + 34, sizeof(int));

    int size = 3 * *width * *height;
    unsigned char* data = malloc(sizeof(char) * size);

    if (!fread(data, sizeof(char), size, f)) {
        return NULL;
    }
    fclose(f);

    /* Swap bytes (image read as BGR > RGB) */
    for (int i = 0; i < imgSize; i += 3) 
    {
        unsigned char tempRGB = data[i];
        data[i] = data[i + 2];
        data[i + 2] = tempRGB;
    }

    return data;
}

typedef struct DDSheader_struct
{
    uint32_t dwSize, dwFlags;
    uint32_t dwHeight, dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    struct DDS_PIXELFORMAT
    {
        uint32_t dwSize, dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
    }
    DDS_PixelFormat;
    
    uint32_t dwCaps1, dwCaps2;
    uint32_t dwReserved2[3];
}
DDS_header;

static inline unsigned char* read_DDS (const char* filename, uint32_t* width, uint32_t* height, uint8_t* numMipmaps)
{
    FILE *fp;
 
    fp = fopen(filename, "rb");  
    if (fp == NULL) {   
        return NULL;
    }
    /* verify the type of file */  
    char filecode[4];
    if (!fread(filecode, 1, 4, fp)) 
    {
        fclose(fp);
        return NULL;
    }
    if (strncmp(filecode, "DDS ", 4) != 0) 
    {
        fclose(fp);    
        return NULL;  
    }  
    /* get the surface desc */  
    DDS_header ddsd;
    if (!fread(&ddsd, sizeof(ddsd), 1, fp)) 
    {
        fclose(fp);
        return NULL;
    }

    /* how big is it going to be including all mipmaps? */  
    uint32_t bufsize = ddsd.dwMipMapCount > 1 ? 
        ddsd.dwPitchOrLinearSize * 2 : ddsd.dwPitchOrLinearSize;  

    unsigned char* data = malloc (bufsize * sizeof(unsigned char));  
    
    if (!fread(data, 1, bufsize, fp)) 
    {
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    *width = ddsd.dwWidth;  
    *height = ddsd.dwHeight;
    *numMipmaps = ddsd.dwMipMapCount;
    //genericImage->components = (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1) ? 3 : 4;

    //uint32_t mipmaps = ddsd.dwMipMapCount;
    uint32_t fourCC  = ddsd.DDS_PixelFormat.dwFourCC;
    uint32_t format = 
        fourCC == from_fourCC ("DXT1") ? 0x83f0 : /* GL_COMPRESSED_RGB_S3TC_DXT1_EXT;  */
        fourCC == from_fourCC ("DXT3") ? 0x83f2 : /* GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; */
        fourCC == from_fourCC ("DXT5") ? 0x83f3 : /* GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; */
        0x0; /* N/A */

    if (!format) {
        free(data);
        return NULL;  
    }

    return data;
}

typedef struct tgaHeader_struct 
{
    //char  IDlength;
    uint8_t  colormapType;
    uint8_t  datatypeCode;
    uint16_t colormapOrigin, colormapLength;
    uint8_t  colormapDepth;
    uint16_t x_origin, y_origin;
    uint16_t width, height;
    uint8_t  bitsperpixel;
    uint8_t  imageDescriptor;
} 
TGA_header;

static inline unsigned char* read_TGA (const char* filename, uint32_t* width, uint32_t* height)
{
    FILE *fp;
 
    fp = fopen(filename, "rb");  
    if (fp == NULL) {   
        return NULL;
    }
    // If The File Header Matches The Uncompressed Header
    TGA_header tgaHeader;
    if (!fread (&tgaHeader, sizeof(char) * 18, 1, fp)) {
        return NULL;
    }

    uint8_t uTGAcompare[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t cTGAcompare[12] = {0, 0, 10,0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    if (memcmp (uTGAcompare, &tgaHeader, sizeof(char) * 12) == 0) {
        printf ("Uncompressed TGA \n");
    }
    if (memcmp (cTGAcompare, &tgaHeader, sizeof(char) * 12) == 0) 
    {        
        //printf ("Compressed TGA \n"); 
        *width   = tgaHeader.width;
        *height  = tgaHeader.height;
        uint8_t bpp  = tgaHeader.bitsperpixel;
    
        uint8_t  bytesPerPixel = (bpp / 8);
        uint32_t pixelCount = tgaHeader.width * tgaHeader.height;
        uint32_t bufsize    = bytesPerPixel * pixelCount;

        unsigned char* data = malloc(bufsize * sizeof(unsigned char));  
        uint32_t currentPixel = 0, currentByte = 0;

        unsigned char* cbuffer = malloc (bytesPerPixel);

        inline void read_TGAcolor (unsigned char* data, uint32_t currentByte, unsigned char cbuffer[],
            uint8_t bytesPerPixel)
        {
            /* Write BGR color trio */
            data[currentByte] = cbuffer[2];
            data[currentByte + 1] = cbuffer[1]; 
            data[currentByte + 2] = cbuffer[0];
            /* Write alpha if BPP is 4 */
            if (bytesPerPixel == 4) {
                data[currentByte + 3] = cbuffer[3];
            }
        }

        while(currentPixel < pixelCount) 
        {
            uint8_t chunkheader = 0;
            if (fread(&chunkheader, sizeof(uint8_t), 1, fp) == 0) {
                return NULL;
            }
            if (!(chunkheader & 128))
            {                                                  
                chunkheader++;
                /* Read pixels from raw packet */
                for (uint16_t i = 0; i < chunkheader; i++)
                {
                    if (fread(cbuffer, 1, bytesPerPixel, fp) != bytesPerPixel) {
                        e_printf("%s", "Error reading!");
                        return NULL;
                    }
                    read_TGAcolor (data, currentByte, cbuffer, bytesPerPixel);
                    currentByte += bytesPerPixel;
                    currentPixel++; 
                }
            }
            else {
                /* set RLE header */
                chunkheader -= 127;
                if (fread(cbuffer, 1, bytesPerPixel, fp) != bytesPerPixel) {
                    e_printf("%s", "Error reading RLE!");
                    return NULL;
                }
                /* read pixels from run-length packet */
                for (uint16_t i = 0; i < chunkheader; i++)
                {
                    read_TGAcolor (data, currentByte, cbuffer, bytesPerPixel);
                    currentByte += bytesPerPixel;
                    currentPixel++; 
                }
            }
        }
        free (cbuffer);
        fclose(fp);
        return data;
    }
    return NULL;
}

#endif