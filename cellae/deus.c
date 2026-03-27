/*
 * deus.c — functiones communes phylo DEI
 */

#include "cella.h"
#include "utilia.h"

void deus_praepara(cella_t *c, const json_par_t *pp, int n)
{
    c->deus.potentia = par_da_int(pp, n, "potentia", 10);
}
