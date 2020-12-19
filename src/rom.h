#include <stdio.h>
#include <stdint.h>
#include "mapper.h"

typedef struct NESrom_struct
{
    uint8_t header[16];
    uint8_t name[4];
    uint8_t valid;

    /* Assume for now fixed PRG and CHR sizes */
    uint8_t prg_data [16 * 1024];
    uint8_t chr_data [8 * 1024];

    /* Each rom has one mapper */
    Mapper mapper;
}
NESrom;

/* Forward declaration */
typedef struct Bus_struct Bus;

void rom_load (Bus * const bus, const char* filename);