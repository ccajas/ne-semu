#include <stdint.h>

/* Forward declaration and wrappers */

typedef struct Mapper_struct Mapper;

typedef struct Mapper_struct
{
    void   *properties;

    uint8_t PRGbanks;
    uint8_t CHRbanks;

    /* Mapper is defined by its read/write implementations */
    uint32_t (*cpuReadWrite)(Mapper*,        uint16_t);
    void     (*ppuReadWrite)(uint16_t const, uint32_t*);
}
Mapper;

Mapper mapper_apply (uint8_t header[], uint16_t const mapperID);

/* Concrete model read/write functions for CPU */   

uint32_t mapper_NROM_cpu_rw  (Mapper * const mapper, uint16_t const address);
uint32_t mapper_MMC1_cpu_rw  (Mapper * const mapper, uint16_t const address);
uint32_t mapper_UxROM_cpu_rw (Mapper * const mapper, uint16_t const address);

/* Concrete model read/write functions for PPU */

void mapper_NROM_ppu_rw  (uint16_t const addr, uint32_t * mapped);
void mapper_MMC1_ppu_rw  (uint16_t const addr, uint32_t * mapped);
void mapper_UxROM_ppu_rw (uint16_t const addr, uint32_t * mapped);