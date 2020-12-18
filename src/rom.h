#include <stdio.h>
#include <stdint.h>

typedef struct NESrom_struct
{
    uint8_t header[16];
    uint8_t name[4];
    uint8_t valid;
    /* Assume for now fixed PRG and CHR sizes */
    uint8_t prg_data [16 * 1024];
    uint8_t chr_data [8 * 1024];
}
NESrom;