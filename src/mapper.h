#include <stdint.h>

/* Forward declaration and wrappers */

typedef struct Mapper_struct Mapper;

typedef struct Mapper_struct
{
    uint8_t PRGbanks;
    uint8_t CHRbanks;

    /* Mapper is defined by its read/write implementations */
    uint32_t (*cpuReadWrite)(Mapper*, uint16_t);
    void     (*ppuReadWrite)();
}
Mapper;

void mapper_implement (Mapper * const mapper, uint16_t const mapperID);

/* Concrete model read/write functions for CPU */   

uint32_t mapper_0_cpu_rw (Mapper * const mapper, uint16_t const address);
uint32_t mapper_2_cpu_rw (Mapper * const mapper, uint16_t const address);

/* Concrete model read/write functions for PPU */

void mapper_0_ppu_rw (uint16_t const addr, uint32_t * mapped);
void mapper_2_ppu_rw (uint16_t const addr, uint32_t * mapped);

// 01234567890abcdef
// 0 1 2 3 4 5 6 7