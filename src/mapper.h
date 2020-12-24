#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include "utils/v_array.h"

/* Forward declaration and wrappers */

typedef struct Mapper_struct Mapper;

typedef struct Mapper_struct
{
    void   *properties;

    uint8_t PRGbanks;
    uint8_t CHRbanks;
    uint8_t bankSelect, lastBankStart;

    /* Access to ROM data */
    struct VArray *PRG;
    struct VArray *CHR;

    /* Mapper is defined by its read/write implementations */
    uint8_t (*read)(Mapper*, uint16_t const, uint8_t);
    void    (*write)(Mapper*, uint16_t const, uint8_t const, uint8_t);
}
Mapper;

Mapper mapper_apply     (uint8_t header[], uint16_t const mapperID);
void   mapper_link_data (Mapper * const mapper, struct VArray * PRG, struct VArray * CHR);

/* Concrete model read functions */   

uint8_t mapper_NROM_read  (Mapper * const mapper, uint16_t const address, uint8_t rw);
uint8_t mapper_MMC1_read  (Mapper * const mapper, uint16_t const address, uint8_t rw);
uint8_t mapper_UxROM_read (Mapper * const mapper, uint16_t const address, uint8_t rw);

/* Concrete model write functions */

void mapper_NROM_write  (Mapper * const mapper, uint16_t const address, uint8_t const data, uint8_t rw);
void mapper_MMC1_write  (Mapper * const mapper, uint16_t const address, uint8_t const data, uint8_t rw);
void mapper_UxROM_write (Mapper * const mapper, uint16_t const address, uint8_t const data, uint8_t rw);

#endif