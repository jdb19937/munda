/*
 * genera.h — tabula constans omnium generum cellularum
 *
 * Pictura, signum, phylum, capacitates per genus.
 * Includitur ab et servitore et clientibus.
 * Nulla dependentia praeter cellula.h.
 */

#ifndef GENERA_H
#define GENERA_H

#include "cellula.h"

typedef struct {
    const char *pictura;
    char signum;
    phylum_t phylum;
    unsigned int capacitates;
} genus_descriptio_t;

static const genus_descriptio_t GENERA[GENERA_NUMERUS] = {
    [VACUUM]  = { "\xC2\xB7 ",         '.', FIXUM,  0 },
    [SAXUM]   = { "\xF0\x9F\xA7\xB1", '#', FIXUM,  0 },
    [FELES]   = { "\xF0\x9F\x90\xB1", 'F', ANIMA,  CAP_ANIMA },
    [DALEKUS] = { "\xF0\x9F\xA4\x96", 'B', ANIMA,  CAP_ANIMA },
    [URSUS]   = { "\xF0\x9F\x90\xBB", 'U', ANIMA,  CAP_ANIMA },
    [MURUS]   = { "\xE2\xAC\x9B",     'W', FIXUM,  0 },
    [RAPUM]   = { "\xF0\x9F\xA5\x95", 'r', CIBUS,  0 },
    [FUNGUS]  = { "\xF0\x9F\x8D\x84", 'f', CIBUS,  0 },
    [ZODUS]   = { "\xE2\x9A\xA1",     'Z', DEI,    CAP_DEI },
    [OCULUS]  = { "I ",                'O', DEI,    CAP_DEI },
    [CORVUS]  = { "\xF0\x9F\x90\xA6", 'C', ANIMA,  CAP_ANIMA },
};

#endif /* GENERA_H */
