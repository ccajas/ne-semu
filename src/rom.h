#include <stdio.h>
#include <stdint.h>
#include "utils/v_array.h"
#include "mapper.h"

typedef struct NESrom_struct
{
    enum mirroringType 
    {
        MIRROR_HORIZONTAL = 0,
        MIRROR_VERTICAL   = 1
    }
    mirroringType;

    uint8_t header[16];
    uint8_t trainer[512];
    uint8_t mirroring;
    uint8_t mapperID;
    uint8_t valid;

    /* Dynamic arrays for PRG and CHR data */
    struct Varray PRGdata;
    struct Varray CHRdata;

    /* Each rom has one mapper */
    Mapper mapper;
}
NESrom;

/* Forward declaration */
typedef struct Bus_struct Bus;

void rom_load  (Bus    * const bus, const char* filename);
void rom_eject (NESrom * const rom);