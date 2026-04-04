/*
 * fictio.h — typi et adiutores pro probatione ficta
 *
 * Provisor fictus vocat genus_ops[g].fictio() pro quaque cellula.
 * Logica ficta in plicis generum ipsis definitur.
 */

#ifndef FICTIO_H
#define FICTIO_H

#include "cellula.h"

#define FICTIO_LATUS_MAX 16

/* vicinitas resoluta in graticulam characterum */
typedef struct fictio_vicinitas {
    char graticula[FICTIO_LATUS_MAX][FICTIO_LATUS_MAX];
    int latus;      /* columnae */
    int series;     /* versus */
    int cx, cy;     /* centrum ("@") */
} fictio_vicinitas_t;

/*
 * quaere proximum signum in vicinitate.
 * reddit directionem cardinalem ad eum, vel DIR_NIHIL si non invenitur.
 */
directio_t fictio_quaere_proximum(const fictio_vicinitas_t *vic, char signum);

/*
 * an signum sit vicinum (distantia 1) in directione data.
 */
int fictio_vicinum_est(
    const fictio_vicinitas_t *vic,
    char signum, directio_t dir
);

/* nomen directionis ("septentrio", "meridies", ...) */
const char *fictio_dir_nomen(directio_t dir);

/* directio fortuita (SEPTENTRIO..ORIENS) */
directio_t fictio_dir_fortuita(void);

#endif /* FICTIO_H */
