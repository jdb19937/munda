/*
 * crispus_internum.h — declarationes internae veli TLS
 *
 * Primitiva cryptographica nunc in arcana/arcana.h habitant.
 * Hic solum TLS-specifica adduntur.
 */

#ifndef CRISPUS_INTERNUM_H
#define CRISPUS_INTERNUM_H

#include "../arcana/arcana.h"

/* --- velum (TLS 1.2) --- */

typedef struct velum velum_t;

velum_t *velum_crea(int fd, const char *hospes);
int      velum_saluta(velum_t *v);
int      velum_scribe(velum_t *v, const void *data, size_t mag);
int      velum_lege(velum_t *v, void *alveus, size_t mag);
void     velum_claude(velum_t *v);

#endif /* CRISPUS_INTERNUM_H */
