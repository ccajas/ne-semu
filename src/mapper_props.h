#include <stdint.h>

struct MapperBase
{
    uint8_t unused;
};

/* MMC1 unique props */

struct MMC1_props
{
    union 
    {
        struct 
        {
            uint8_t MIRRORING     : 2;
            uint8_t PRG_BANK_MODE : 2;
            uint8_t CHR_BANK_MODE : 1;
            uint8_t UNUSED        : 3;
        };
        uint8_t flags;
    } 
    control;

    uint8_t shiftReg;
    uint8_t PRGram[0x2000];
    uint8_t CHRbank[2];
    uint8_t PRGbank;
};