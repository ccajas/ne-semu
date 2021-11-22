#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include "utils/v_array.h"
#include "mapper_props.h"

#define NUM_MAPPERS 3

/* Forward declaration and wrappers */

typedef struct Mapper_struct Mapper;

typedef struct Mapper_struct
{
    /* Pointer to mapper-specific properties */
    void   *props;

    uint8_t PRGbanks;
    uint8_t CHRbanks;
    uint8_t bankSelect, lastBankStart;
    uint8_t usesCHR;

    /* Access to ROM data */
    struct VArray *PRG;
    struct VArray *CHR;
    struct VArray localCHR;

    /* Mapper is defined by its read/write implementations */
    uint8_t (*read) (Mapper*, uint16_t const, uint8_t);
    void    (*write)(Mapper*, uint16_t const, uint8_t const, uint8_t);
}
Mapper;

Mapper mapper_apply (uint8_t header[], uint16_t const mapperID);

/* Concrete model read functions */

uint8_t mapper_NROM_read  (Mapper * mapper, uint16_t const address, uint8_t rw);
uint8_t mapper_MMC1_read  (Mapper * mapper, uint16_t const address, uint8_t rw);
uint8_t mapper_UxROM_read (Mapper * mapper, uint16_t const address, uint8_t rw);

extern uint8_t (* mapperRead[NUM_MAPPERS])(Mapper*, uint16_t, uint8_t);

/* Concrete model write functions */

void mapper_NROM_write  (Mapper * mapper, uint16_t const address, uint8_t const data, uint8_t rw);
void mapper_MMC1_write  (Mapper * mapper, uint16_t const address, uint8_t const data, uint8_t rw);
void mapper_UxROM_write (Mapper * mapper, uint16_t const address, uint8_t const data, uint8_t rw);

extern void (* mapperWrite[NUM_MAPPERS])(Mapper*, uint16_t, uint8_t, uint8_t);

#endif
