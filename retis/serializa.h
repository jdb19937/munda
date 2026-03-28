/*
 * serializa.h — serialisatio tabulae servitoris
 *
 * Solum a servitore includitur. Pendet a tabula.h.
 */

#ifndef SERIALIZA_H
#define SERIALIZA_H

#include "tabula.h"

/* serializa tabulam ad JSON; vocans debet liberare reditum */
char *tabula_ad_json(const tabula_t *tab, unsigned long gradus);

#endif /* SERIALIZA_H */
